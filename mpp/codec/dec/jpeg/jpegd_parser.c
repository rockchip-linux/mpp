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

#define MODULE_TAG "jpegd_parser"

#include "mpp_bitread.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_packet_impl.h"

#include "jpegd_api.h"
#include "jpegd_parser.h"


#define STRM_ERROR 0xFFFFFFFFU
RK_U32 jpegd_log = 0;

/*------------------------------------------------------------------------------
        Function name: jpegd_get_byte

        Functional description:
          Reads one byte (8 bits) from stream and returns the bits

          Note! This function does not skip the 0x00 byte if the previous
          byte value was 0xFF!!!

        Inputs:
          StreamStorage *pStream   Pointer to structure

        Outputs:
          Returns 8 bit value if ok
          else returns STRM_ERROR (0xFFFFFFFF)
------------------------------------------------------------------------------*/
RK_U32 jpegd_get_byte(StreamStorage * pStream)
{
    RK_U32 tmp;

    if ((pStream->readBits + 8) > (8 * pStream->streamLength))
        return (STRM_ERROR);

    tmp = *(pStream->pCurrPos)++;
    tmp = (tmp << 8) | *(pStream->pCurrPos);
    tmp = (tmp >> (8 - pStream->bitPosInByte)) & 0xFF;
    pStream->readBits += 8;
    return (tmp);
}

/*------------------------------------------------------------------------------
        Function name: jpegd_get_two_bytes

        Functional description:
          Reads two bytes (16 bits) from stream and returns the bits

          Note! This function does not skip the 0x00 byte if the previous
          byte value was 0xFF!!!

        Inputs:
          StreamStorage *pStream   Pointer to structure

        Outputs:
          Returns 16 bit value
------------------------------------------------------------------------------*/
RK_U32 jpegd_get_two_bytes(StreamStorage * pStream)
{
    RK_U32 tmp;

    if ((pStream->readBits + 16) > (8 * pStream->streamLength))
        return (STRM_ERROR);

    tmp = *(pStream->pCurrPos)++;
    tmp = (tmp << 8) | *(pStream->pCurrPos)++;
    tmp = (tmp << 8) | *(pStream->pCurrPos);
    tmp = (tmp >> (8 - pStream->bitPosInByte)) & 0xFFFF;
    pStream->readBits += 16;
    return (tmp);
}

/*------------------------------------------------------------------------------
        Function name: jpegd_show_bits

        Functional description:
          Reads 32 bits from stream and returns the bits, does not update
          stream pointers. If there are not enough bits in data buffer it
          reads the rest of the data buffer bits and fills the lsb of return
          value with zero bits.

          Note! This function will skip the byte valued 0x00 if the previous
          byte value was 0xFF!!!

        Inputs:
          StreamStorage *pStream   Pointer to structure

        Outputs:
          Returns  32 bit value
------------------------------------------------------------------------------*/
RK_U32 jpegd_show_bits(StreamStorage * pStream)
{
    RK_S32 bits;
    RK_U32 readBits;
    RK_U32 out = 0;
    RK_U8 *pData = pStream->pCurrPos;

    /* bits left in buffer */
    bits = (RK_S32) (8 * pStream->streamLength - pStream->readBits);
    if (!bits)
        return (0);

    readBits = 0;
    do {
        if (pData > pStream->pStartOfStream) {
            /* FF 00 bytes in stream -> jump over 00 byte */
            if ((pData[-1] == 0xFF) && (pData[0] == 0x00)) {
                pData++;
                bits -= 8;
            }
        }
        if (readBits == 32 && pStream->bitPosInByte) {
            out <<= pStream->bitPosInByte;
            out |= *pData >> (8 - pStream->bitPosInByte);
            readBits = 0;
            break;
        }
        out = (out << 8) | *pData++;
        readBits += 8;
        bits -= 8;
    } while (readBits < (32 + pStream->bitPosInByte) && bits > 0);

    if (bits <= 0 &&
        ((readBits + pStream->readBits) >= (pStream->streamLength * 8))) {
        /* not enough bits in stream, fill with zeros */
        out = (out << (32 - (readBits - pStream->bitPosInByte)));
    }

    return (out);
}

/*------------------------------------------------------------------------------
        Function name: jpegd_flush_bits

        Functional description:
          Updates stream pointers, flushes bits from stream

          Note! This function will skip the byte valued 0x00 if the previous
          byte value was 0xFF!!!

        Inputs:
          StreamStorage *pStream   Pointer to structure
          RK_U32 bits                 Number of bits to be flushed

        Outputs:
          OK
          STRM_ERROR
------------------------------------------------------------------------------*/
RK_U32 jpegd_flush_bits(StreamStorage * pStream, RK_U32 bits)
{
    RK_U32 tmp;
    RK_U32 extraBits = 0;

    if ((pStream->readBits + bits) > (8 * pStream->streamLength)) {
        /* there are not so many bits left in buffer */
        /* stream pointers to the end of the stream  */
        /* and return value STRM_ERROR               */
        pStream->readBits = 8 * pStream->streamLength;
        pStream->bitPosInByte = 0;
        pStream->pCurrPos = pStream->pStartOfStream + pStream->streamLength;
        return (STRM_ERROR);
    } else {
        tmp = 0;
        while (tmp < bits) {
            if (bits - tmp < 8) {
                if ((8 - pStream->bitPosInByte) > (bits - tmp)) {
                    /* inside one byte */
                    pStream->bitPosInByte += bits - tmp;
                    tmp = bits;
                } else {
                    if (pStream->pCurrPos[0] == 0xFF &&
                        pStream->pCurrPos[1] == 0x00) {
                        extraBits += 8;
                        pStream->pCurrPos += 2;
                    } else {
                        pStream->pCurrPos++;
                    }
                    tmp += 8 - pStream->bitPosInByte;
                    pStream->bitPosInByte = 0;
                    pStream->bitPosInByte = bits - tmp;
                    tmp = bits;
                }
            } else {
                tmp += 8;
                if (pStream->appnFlag) {
                    pStream->pCurrPos++;
                } else {
                    if (pStream->pCurrPos[0] == 0xFF &&
                        pStream->pCurrPos[1] == 0x00) {
                        extraBits += 8;
                        pStream->pCurrPos += 2;
                    } else {
                        pStream->pCurrPos++;
                    }
                }
            }
        }
        /* update stream pointers */
        pStream->readBits += bits + extraBits;
        return (MPP_OK);
    }
}

JpegDecRet jpegd_set_yuv_mode(JpegSyntaxParam *pSyntax)
{
    /*  check input format */
    if (pSyntax->frame.Nf == 3) {
        if (pSyntax->frame.component[0].H == 2 &&
            pSyntax->frame.component[0].V == 2 &&
            pSyntax->frame.component[1].H == 1 &&
            pSyntax->frame.component[1].V == 1 &&
            pSyntax->frame.component[2].H == 1 &&
            pSyntax->frame.component[2].V == 1) {
            JPEGD_INFO_LOG("YCbCr Format: YUV420(2*2:1*1:1*1)");
            pSyntax->info.yCbCrMode = JPEGDEC_YUV420;
            pSyntax->info.X = pSyntax->frame.hwX;
            pSyntax->info.Y = pSyntax->frame.hwY;
        } else if (pSyntax->frame.component[0].H == 2 && pSyntax->frame.component[0].V == 1 &&
                   pSyntax->frame.component[1].H == 1 && pSyntax->frame.component[1].V == 1 &&
                   pSyntax->frame.component[2].H == 1 && pSyntax->frame.component[2].V == 1) {
            JPEGD_INFO_LOG("YCbCr Format: YUV422(%d*%d:%d*%d:%d*%d)",
                           pSyntax->frame.component[0].H, pSyntax->frame.component[0].V,
                           pSyntax->frame.component[1].H, pSyntax->frame.component[1].V,
                           pSyntax->frame.component[2].H, pSyntax->frame.component[2].V);
            pSyntax->info.yCbCrMode = JPEGDEC_YUV422;
            pSyntax->info.X = (pSyntax->frame.hwX);
            pSyntax->info.Y = (pSyntax->frame.hwY);

            /* check if fill needed */
            if ((pSyntax->frame.Y & 0xF) && (pSyntax->frame.Y & 0xF) <= 8)
                pSyntax->info.fillBottom = 1;
        } else if (pSyntax->frame.component[0].H == 1 &&
                   pSyntax->frame.component[0].V == 2 &&
                   pSyntax->frame.component[1].H == 1 &&
                   pSyntax->frame.component[1].V == 1 &&
                   pSyntax->frame.component[2].H == 1 &&
                   pSyntax->frame.component[2].V == 1) {
            JPEGD_INFO_LOG("YCbCr Format: YUV440(1*2:1*1:1*1)");
            pSyntax->info.yCbCrMode = JPEGDEC_YUV440;
            pSyntax->info.X = (pSyntax->frame.hwX);
            pSyntax->info.Y = (pSyntax->frame.hwY);

            /* check if fill needed */
            if ((pSyntax->frame.X & 0xF) && (pSyntax->frame.X & 0xF) <= 8)
                pSyntax->info.fillRight = 1;
        }
        /* JPEG_YCBCR444 : NOT SUPPORTED */
        else if (pSyntax->frame.component[0].H == 1 &&
                 pSyntax->frame.component[0].V == 1 &&
                 pSyntax->frame.component[1].H == 1 &&
                 pSyntax->frame.component[1].V == 1 &&
                 pSyntax->frame.component[2].H == 1 &&
                 pSyntax->frame.component[2].V == 1) {
            JPEGD_INFO_LOG("YCbCr Format: YUV444(1*1:1*1:1*1)");
            pSyntax->info.yCbCrMode = JPEGDEC_YUV444;
            pSyntax->info.X = pSyntax->frame.hwX;
            pSyntax->info.Y = pSyntax->frame.hwY;

            /* check if fill needed */
            if ((pSyntax->frame.X & 0xF) && (pSyntax->frame.X & 0xF) <= 8)
                pSyntax->info.fillRight = 1;

            if ((pSyntax->frame.Y & 0xF) && (pSyntax->frame.Y & 0xF) <= 8)
                pSyntax->info.fillBottom = 1;
        } else if (pSyntax->frame.component[0].H == 4 &&
                   pSyntax->frame.component[0].V == 1 &&
                   pSyntax->frame.component[1].H == 1 &&
                   pSyntax->frame.component[1].V == 1 &&
                   pSyntax->frame.component[2].H == 1 &&
                   pSyntax->frame.component[2].V == 1) {
            JPEGD_INFO_LOG("YCbCr Format: YUV411(4*1:1*1:1*1)");
            pSyntax->info.yCbCrMode = JPEGDEC_YUV411;
            pSyntax->info.X = (pSyntax->frame.hwX);
            pSyntax->info.Y = (pSyntax->frame.hwY);

            /* check if fill needed */
            if ((pSyntax->frame.Y & 0xF) && (pSyntax->frame.Y & 0xF) <= 8)
                pSyntax->info.fillBottom = 1;
        } else {
            JPEGD_ERROR_LOG("Unsupported YCbCr Format: (%d*%d:%d*%d:%d*%d)", pSyntax->frame.component[0].H, pSyntax->frame.component[0].V,
                            pSyntax->frame.component[1].H, pSyntax->frame.component[1].V,
                            pSyntax->frame.component[2].H, pSyntax->frame.component[2].V);
            return (JPEGDEC_UNSUPPORTED);
        }
    } else if (pSyntax->frame.Nf == 1) {
        /* 4:0:0 */
        if ((pSyntax->frame.component[0].V == 1) ||
            (pSyntax->frame.component[0].H == 1)) {
            pSyntax->info.yCbCrMode = JPEGDEC_YUV400;
            pSyntax->info.X = (pSyntax->frame.hwX);
            pSyntax->info.Y = (pSyntax->frame.hwY);

            /* check if fill needed */
            if ((pSyntax->frame.X & 0xF) && (pSyntax->frame.X & 0xF) <= 8)
                pSyntax->info.fillRight = 1;

            if ((pSyntax->frame.Y & 0xF) && (pSyntax->frame.Y & 0xF) <= 8)
                pSyntax->info.fillBottom = 1;
        } else {
            JPEGD_ERROR_LOG("unsupported format");
            return (JPEGDEC_UNSUPPORTED);
        }
    } else {
        JPEGD_ERROR_LOG("unsupported format");
        return (JPEGDEC_UNSUPPORTED);
    }

    /* save the original sampling format for progressive use */
    pSyntax->info.yCbCrModeOrig = pSyntax->info.yCbCrMode;

    return (JPEGDEC_OK);
}

JpegDecRet jpegd_decode_frame_header(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    RK_U32 i;
    RK_U32 width, height;
    RK_U32 tmp1, tmp2;
    RK_U32 Hmax = 0;
    RK_U32 Vmax = 0;
    RK_U32 size = 0;
    JpegDecRet retCode;

    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    StreamStorage *pStream = &(pSyntax->stream);

    retCode = JPEGDEC_OK;

    /* frame header length */
    pSyntax->frame.Lf = jpegd_get_two_bytes(pStream);

    /* check if there is enough data */
    if (((pSyntax->stream.readBits / 8) + pSyntax->frame.Lf) >
        pSyntax->stream.streamLength)
        return (JPEGDEC_STRM_ERROR);

    /* Sample precision */
    pSyntax->frame.P = jpegd_get_byte(pStream);
    if (pSyntax->frame.P != 8) {
        JPEGD_ERROR_LOG("frame.P(%d) is not supported", pSyntax->frame.P);
        return (JPEGDEC_UNSUPPORTED);
    }
    /* Number of Lines */
    pSyntax->frame.Y = jpegd_get_two_bytes(pStream);
    if (pSyntax->frame.Y < 1) {
        return (JPEGDEC_UNSUPPORTED);
    }
    pSyntax->frame.hwY = pSyntax->frame.Y;

    /* round up to next multiple-of-16 */
    pSyntax->frame.hwY += 0xf;
    pSyntax->frame.hwY &= ~(0xf);
    JPEGD_INFO_LOG("Height, frame.Y:%d, frame.hwY:%d", pSyntax->frame.Y, pSyntax->frame.hwY);

    /* Number of samples per line */
    pSyntax->frame.X = jpegd_get_two_bytes(pStream);
    if (pSyntax->frame.X < 1) {
        return (JPEGDEC_UNSUPPORTED);
    }
    pSyntax->frame.hwX = pSyntax->frame.X;

    /* round up to next multiple-of-16 */
    pSyntax->frame.hwX += 0xf;
    pSyntax->frame.hwX &= ~(0xf);
    JPEGD_INFO_LOG("Width, frame.X:%d, frame.hwX:%d", pSyntax->frame.X, pSyntax->frame.hwX);

    /* for internal() */
    pSyntax->info.X = pSyntax->frame.hwX;
    pSyntax->info.Y = pSyntax->frame.hwY;

    /* check for minimum and maximum dimensions */
    if (pSyntax->frame.hwX < pCtx->minSupportedWidth ||
        pSyntax->frame.hwY < pCtx->minSupportedHeight ||
        pSyntax->frame.hwX > pCtx->maxSupportedWidth ||
        pSyntax->frame.hwY > pCtx->maxSupportedHeight ||
        (pSyntax->frame.hwX * pSyntax->frame.hwY) > pCtx->maxSupportedPixelAmount) {
        JPEGD_ERROR_LOG("FRAME(%d*%d): Unsupported size\n", pSyntax->frame.hwX, pSyntax->frame.hwY);
        return (JPEGDEC_UNSUPPORTED);
    }

    /* Number of components */
    pSyntax->frame.Nf = jpegd_get_byte(pStream);
    if ((pSyntax->frame.Nf != 3) && (pSyntax->frame.Nf != 1)) {
        JPEGD_ERROR_LOG("frame.Nf(%d) is unsupported", pSyntax->frame.Nf);
        return (JPEGDEC_UNSUPPORTED);
    }

    /* save component specific data */
    /* Nf == number of components */
    for (i = 0; i < pSyntax->frame.Nf; i++) {
        pSyntax->frame.component[i].C = jpegd_get_byte(pStream);
        if (i == 0) { /* for the first component */
            /* if first component id is something else than 1 (jfif) */
            pSyntax->scan.index = pSyntax->frame.component[i].C;
        } else {
            /* if component ids 'jumps' */
            if ((pSyntax->frame.component[i - 1].C + 1) != pSyntax->frame.component[i].C) {
                JPEGD_ERROR_LOG("component ids 'jumps'\n");
                return (JPEGDEC_UNSUPPORTED);
            }
        }
        tmp1 = jpegd_get_byte(pStream);
        pSyntax->frame.component[i].H = tmp1 >> 4;
        if (pSyntax->frame.component[i].H > Hmax) {
            Hmax = pSyntax->frame.component[i].H;
        }
        pSyntax->frame.component[i].V = tmp1 & 0xF;
        if (pSyntax->frame.component[i].V > Vmax) {
            Vmax = pSyntax->frame.component[i].V;
        }

        pSyntax->frame.component[i].Tq = jpegd_get_byte(pStream);
    }

    if (pSyntax->frame.Nf == 1) {
        Hmax = Vmax = 1;
        pSyntax->frame.component[0].H = 1;
        pSyntax->frame.component[0].V = 1;
    } else if (Hmax == 0 || Vmax == 0) {
        JPEGD_ERROR_LOG("Hmax(%d),Vmax(%d) is unsupported", Hmax, Vmax);
        return (JPEGDEC_UNSUPPORTED);
    }

    /* JPEG_YCBCR411 horizontal size has to be multiple of 32 pels */
    if (Hmax == 4 && (pSyntax->frame.hwX & 0x1F)) {
        /* round up to next multiple-of-32 */
        pSyntax->frame.hwX += 16;
        pSyntax->info.X = pSyntax->frame.hwX;

        /* check for minimum and maximum dimensions */
        if (pSyntax->frame.hwX > pCtx->maxSupportedWidth ||
            (pSyntax->frame.hwX * pSyntax->frame.hwY) > pCtx->maxSupportedPixelAmount) {
            JPEGD_ERROR_LOG("FRAME(%d*%d): Unsupported size\n", pSyntax->frame.hwX, pSyntax->frame.hwY);
            return (JPEGDEC_UNSUPPORTED);
        }
    }

    /* set image pointers, calculate pixelPerRow for each component */
    width = ((pSyntax->frame.hwX + Hmax * 8 - 1) / (Hmax * 8)) * Hmax * 8;
    height = ((pSyntax->frame.hwY + Vmax * 8 - 1) / (Vmax * 8)) * Vmax * 8;
    JPEGD_VERBOSE_LOG("width:%d, height:%d", width, height);

    /* calculate numMcuInRow and numMcuInFrame */
    JPEGD_ASSERT(Hmax != 0);
    JPEGD_ASSERT(Vmax != 0);
    pSyntax->frame.numMcuInRow = width / (8 * Hmax);
    pSyntax->frame.numMcuInFrame = pSyntax->frame.numMcuInRow * (height / (8 * Vmax));
    JPEGD_VERBOSE_LOG("numMcuInRow:%d, numMcuInFrame:%d", pSyntax->frame.numMcuInRow, pSyntax->frame.numMcuInFrame);

    /* reset mcuNumbers */
    pSyntax->frame.mcuNumber = 0;
    pSyntax->frame.row = pSyntax->frame.col = 0;

    for (i = 0; i < pSyntax->frame.Nf; i++) {
        JPEGD_ASSERT(i <= 2);
        tmp1 = (width * pSyntax->frame.component[i].H + Hmax - 1) / Hmax;
        tmp2 = (height * pSyntax->frame.component[i].V + Vmax - 1) / Vmax;
        size += tmp1 * tmp2;

        /* pixels per row */
        pSyntax->image.pixelsPerRow[i] = tmp1;
        pSyntax->image.columns[i] = tmp2;
        pSyntax->frame.numBlocks[i] =
            (((pSyntax->frame.hwX * pSyntax->frame.component[i].H) / Hmax + 7) >> 3) *
            (((pSyntax->frame.hwY * pSyntax->frame.component[i].V) / Vmax + 7) >> 3);

        if (i == 0) {
            pSyntax->image.sizeLuma = size;
        }
    }

    pSyntax->image.size = size;
    pSyntax->image.sizeChroma = size - pSyntax->image.sizeLuma;
    JPEGD_VERBOSE_LOG("sizeLuma:%d", pSyntax->image.sizeLuma);
    JPEGD_VERBOSE_LOG("image.size:%d, sizeChroma:%d", pSyntax->image.size, pSyntax->image.sizeChroma);

    /* set YUV mode & calculate rlc tmp size */
    retCode = jpegd_set_yuv_mode(pSyntax);
    if (retCode != JPEGDEC_OK) {
        JPEGD_ERROR_LOG("set YUV mode failed");
        return (retCode);
    }

    return (JPEGDEC_OK);
    FUN_TEST("Exit");
}

JpegDecRet jpegd_decode_scan_header(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    RK_U32 i;
    RK_U32 tmp;

    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    StreamStorage *pStream = &(pSyntax->stream);

    pSyntax->scan.Ls = jpegd_get_two_bytes(pStream);

    /* check if there is enough data */
    if (((pStream->readBits / 8) + pSyntax->scan.Ls) > pStream->streamLength) {
        JPEGD_ERROR_LOG("no enough data");
        return (JPEGDEC_STRM_ERROR);
    }

    pSyntax->scan.Ns = jpegd_get_byte(pStream);

    pSyntax->info.fillX = pSyntax->info.fillY = 0;
    if (pSyntax->scan.Ns == 1) {
        JPEGD_INFO_LOG("scan.Ns == 1");
        /* Reset to non-interleaved baseline operation type */
        if (pSyntax->info.operationType == JPEGDEC_BASELINE &&
            pSyntax->info.yCbCrMode != JPEGDEC_YUV400)
            pSyntax->info.operationType = JPEGDEC_NONINTERLEAVED;

        tmp = jpegd_get_byte(pStream);
        pSyntax->frame.cIndex = tmp - 1;
        pSyntax->info.componentId = pSyntax->frame.cIndex;
        pSyntax->scan.Cs[pSyntax->frame.cIndex] = tmp;
        tmp = jpegd_get_byte(pStream);
        pSyntax->scan.Td[pSyntax->frame.cIndex] = tmp >> 4;
        pSyntax->scan.Ta[pSyntax->frame.cIndex] = tmp & 0x0F;

        /* check/update component info */
        if (pSyntax->frame.Nf == 3) {
            pSyntax->info.fillRight = 0;
            pSyntax->info.fillBottom = 0;
            pSyntax->info.pfNeeded[pSyntax->scan.Cs[pSyntax->frame.cIndex] - 1] = 0;
            if (pSyntax->scan.Cs[pSyntax->frame.cIndex] == 2 ||
                pSyntax->scan.Cs[pSyntax->frame.cIndex] == 3) {
                if (pSyntax->info.operationType == JPEGDEC_PROGRESSIVE ||
                    pSyntax->info.operationType == JPEGDEC_NONINTERLEAVED ||
                    pSyntax->info.nonInterleavedScanReady) {
                    if (pSyntax->info.yCbCrModeOrig == JPEGDEC_YUV420) {
                        pSyntax->info.X = pSyntax->frame.hwX >> 1;
                        pSyntax->info.Y = pSyntax->frame.hwY >> 1;
                    } else if (pSyntax->info.yCbCrModeOrig == JPEGDEC_YUV422) {
                        pSyntax->info.X = pSyntax->frame.hwX >> 1;
                    } else if (pSyntax->info.yCbCrModeOrig == JPEGDEC_YUV440) {
                        pSyntax->info.Y = pSyntax->frame.hwY >> 1;
                    } else {
                        pSyntax->info.yCbCrMode = 0;
                        return (JPEGDEC_UNSUPPORTED);
                    }
                }

                pSyntax->info.yCbCrMode = 0;
            } else if (pSyntax->scan.Cs[pSyntax->frame.cIndex] == 1) { /* YCbCr 4:2:0 */
                pSyntax->info.X = pSyntax->frame.hwX;
                pSyntax->info.Y = pSyntax->frame.hwY;
                if (pSyntax->info.yCbCrMode == JPEGDEC_YUV420) {
                    pSyntax->info.yCbCrMode = 1;
                } else if (pSyntax->info.yCbCrMode == JPEGDEC_YUV422) {
                    pSyntax->info.yCbCrMode = 0;
                    if (pSyntax->frame.cIndex == 0) {
                        JPEGD_INFO_LOG("SCAN: #YUV422 FLAG\n");
                        pSyntax->info.yCbCr422 = 1;
                    }
                } else if (pSyntax->info.yCbCrMode == JPEGDEC_YUV444) {
                    pSyntax->info.yCbCrMode = 0;
                    return (JPEGDEC_UNSUPPORTED);
                }
            } else {
                pSyntax->info.yCbCrMode = 0;
                return (JPEGDEC_UNSUPPORTED);
            }

            if (pSyntax->info.X & 0xF) {
                pSyntax->info.X += 8;
                pSyntax->info.fillX = 1;
            } else if ((pSyntax->scan.Cs[pSyntax->frame.cIndex] == 1 ||
                        pSyntax->info.yCbCrModeOrig == JPEGDEC_YUV440) &&
                       (pSyntax->frame.X & 0xF) && (pSyntax->frame.X & 0xF) <= 8) {
                pSyntax->info.fillRight = 1;
            }

            if (pSyntax->info.Y & 0xF) {
                pSyntax->info.Y += 8;
                pSyntax->info.fillY = 1;
            } else if ((pSyntax->scan.Cs[pSyntax->frame.cIndex] == 1 ||
                        pSyntax->info.yCbCrModeOrig == JPEGDEC_YUV422) &&
                       (pSyntax->frame.Y & 0xF) && (pSyntax->frame.Y & 0xF) <= 8) {
                pSyntax->info.fillBottom = 1;
            }
        } else if (pSyntax->frame.Nf == 1) {
            JPEGD_INFO_LOG("SCAN: #YUV422 FLAG\n");
            pSyntax->info.yCbCr422 = 0;
        }

        /* decoding info */
        if (pSyntax->info.operationType == JPEGDEC_PROGRESSIVE ||
            pSyntax->info.operationType == JPEGDEC_NONINTERLEAVED) {
            pSyntax->info.yCbCrMode = 0;
        }
    } else {
        for (i = 0; i < pSyntax->scan.Ns; i++) {
            pSyntax->scan.Cs[i] = jpegd_get_byte(pStream);
            tmp = jpegd_get_byte(pStream);
            pSyntax->scan.Td[i] = tmp >> 4; /* which DC table */
            pSyntax->scan.Ta[i] = tmp & 0x0F;   /* which AC table */
            pSyntax->info.pfNeeded[i] = 1;
        }
        pSyntax->info.X = pSyntax->frame.hwX;
        pSyntax->info.Y = pSyntax->frame.hwY;
        pSyntax->frame.cIndex = 0;
        pSyntax->info.yCbCrMode = pSyntax->info.yCbCrModeOrig;
    }

    pSyntax->scan.Ss = jpegd_get_byte(pStream);
    pSyntax->scan.Se = jpegd_get_byte(pStream);
    tmp = jpegd_get_byte(pStream);
    pSyntax->scan.Ah = tmp >> 4;
    pSyntax->scan.Al = tmp & 0x0F;

    if (pSyntax->frame.codingType == SOF0) {
        /* baseline */
        if (pSyntax->scan.Ss != 0)
            return (JPEGDEC_UNSUPPORTED);
        if (pSyntax->scan.Se != 63)
            return (JPEGDEC_UNSUPPORTED);
        if (pSyntax->scan.Ah != 0)
            return (JPEGDEC_UNSUPPORTED);
        if (pSyntax->scan.Al != 0)
            return (JPEGDEC_UNSUPPORTED);

        /* update scan decoding parameters */
        /* interleaved/non-interleaved */
        if (pSyntax->info.operationType == JPEGDEC_BASELINE)
            pSyntax->info.nonInterleaved = 0;
        else
            pSyntax->info.nonInterleaved = 1;
        /* decoding info */
        if ((pSyntax->frame.Nf == 3 && pSyntax->scan.Ns == 1) ||
            (pSyntax->frame.Nf == 1 && pSyntax->scan.Ns == 1))
            pSyntax->info.amountOfQTables = 1;
        else
            pSyntax->info.amountOfQTables = 3;
    }

    if (pSyntax->frame.codingType == SOF2) {
        /* progressive */
        if (pSyntax->scan.Ss == 0 && pSyntax->scan.Se != 0)
            return (JPEGDEC_UNSUPPORTED);
        if (/*pSyntax->scan.Ah < 0 ||*/ pSyntax->scan.Ah > 13)
            return (JPEGDEC_UNSUPPORTED);
        if (/*pSyntax->scan.Al < 0 ||*/ pSyntax->scan.Al > 13)
            return (JPEGDEC_UNSUPPORTED);

        /* update scan decoding parameters */
        /* TODO! What if 2 components, possible??? */
        /* interleaved/non-interleaved */
        if (pSyntax->scan.Ns == 1) {
            pSyntax->info.nonInterleaved = 1;
            /* component ID */
            pSyntax->info.componentId = pSyntax->frame.cIndex;
            pSyntax->info.amountOfQTables = 1;
        } else {
            pSyntax->info.nonInterleaved = 0;
            /* component ID ==> set to luma ==> interleaved */
            pSyntax->info.componentId = 0;
            pSyntax->info.amountOfQTables = 3;
        }

    }

    FUN_TEST("Exit");
    return (JPEGDEC_OK);
}

JpegDecRet jpegd_decode_scan(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    JpegDecRet retCode;
    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    retCode = JPEGDEC_ERROR;

    retCode = jpegd_decode_scan_header(pCtx);
    if (retCode != JPEGDEC_OK) {
        JPEGD_ERROR_LOG("Decode Scan Header Failed");
        return (retCode);
    }

    JPEGD_VERBOSE_LOG("SCAN: MODE: %d\n", pSyntax->frame.codingType);
    FUN_TEST("Exit");
    return (JPEGDEC_OK);
}

JpegDecRet jpegd_decode_huffman_tables(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    RK_U32 i, len, Tc, Th, tmp;
    RK_S32 j;
    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    StreamStorage *pStream = &(pSyntax->stream);

    pSyntax->vlc.Lh = jpegd_get_two_bytes(pStream);

    /* check if there is enough data for tables */
    if (((pStream->readBits / 8) + pSyntax->vlc.Lh) > pStream->streamLength) {
        JPEGD_ERROR_LOG("no enough data for tables");
        return (JPEGDEC_STRM_ERROR);
    }

    /* four bytes already read in */
    len = 4;

    while (len < pSyntax->vlc.Lh) {
        tmp = jpegd_get_byte(pStream);
        len++;
        Tc = tmp >> 4;  /* Table class: DC or AC */
        if (Tc != 0 && Tc != 1) {
            JPEGD_ERROR_LOG("Tc(%d) is unsupported", Tc);
            return (JPEGDEC_UNSUPPORTED);
        }
        Th = tmp & 0xF; /* Huffman table identifier: 0 or 1 .(Baseline)*/
        /* only two tables in baseline allowed */
        if ((pSyntax->frame.codingType == SOF0) && (Th > 1)) {
            JPEGD_ERROR_LOG("Th(%d) is unsupported", Th);
            return (JPEGDEC_UNSUPPORTED);
        }
        /* four tables in progressive allowed */
        if ((pSyntax->frame.codingType == SOF2) && (Th > 3)) {
            JPEGD_ERROR_LOG("Th(%d) is unsupported", Th);
            return (JPEGDEC_UNSUPPORTED);
        }

        /* set the table pointer */
        if (Tc) {
            /* AC table */
            switch (Th) {
            case 0:
                JPEGD_VERBOSE_LOG("ac0\n");
                pSyntax->vlc.table = &(pSyntax->vlc.acTable0);
                break;
            case 1:
                JPEGD_VERBOSE_LOG("ac1\n");
                pSyntax->vlc.table = &(pSyntax->vlc.acTable1);
                break;
            case 2:
                JPEGD_VERBOSE_LOG("ac2\n");
                pSyntax->vlc.table = &(pSyntax->vlc.acTable2);
                break;
            case 3:
                JPEGD_VERBOSE_LOG("ac3\n");
                pSyntax->vlc.table = &(pSyntax->vlc.acTable3);
                break;
            default:
                JPEGD_ERROR_LOG("Th(%d) is unsupported", Th);
                return (JPEGDEC_UNSUPPORTED);
            }
        } else {
            /* DC table */
            switch (Th) {
            case 0:
                JPEGD_VERBOSE_LOG("dc0\n");
                pSyntax->vlc.table = &(pSyntax->vlc.dcTable0);
                break;
            case 1:
                JPEGD_VERBOSE_LOG("dc1\n");
                pSyntax->vlc.table = &(pSyntax->vlc.dcTable1);
                break;
            case 2:
                JPEGD_VERBOSE_LOG("dc2\n");
                pSyntax->vlc.table = &(pSyntax->vlc.dcTable2);
                break;
            case 3:
                JPEGD_VERBOSE_LOG("dc3\n");
                pSyntax->vlc.table = &(pSyntax->vlc.dcTable3);
                break;
            default:
                JPEGD_ERROR_LOG("Th(%d) is unsupported", Th);
                return (JPEGDEC_UNSUPPORTED);
            }
        }

        tmp = 0;
        /* read in the values of list BITS */
        for (i = 0; i < 16; i++) {
            tmp += pSyntax->vlc.table->bits[i] = jpegd_get_byte(pStream);
            len++;
        }
        /* allocate memory for HUFFVALs */
        if (pSyntax->vlc.table->vals != NULL) {
            /* free previously reserved table */
            mpp_free(pSyntax->vlc.table->vals);
        }

        pSyntax->vlc.table->vals = (RK_U32 *) mpp_calloc(RK_U32, tmp);

        /* set the table length */
        pSyntax->vlc.table->tableLength = tmp;
        /* read in the HUFFVALs */
        for (i = 0; i < tmp; i++) {
            pSyntax->vlc.table->vals[i] = jpegd_get_byte(pStream);
            len++;
        }
        /* first and last lengths */
        for (i = 0; i < 16; i++) {
            if (pSyntax->vlc.table->bits[i] != 0) {
                pSyntax->vlc.table->start = i;
                break;
            }
        }
        for (j = 15; j >= 0; j--) {
            if (pSyntax->vlc.table->bits[j] != 0) {
                pSyntax->vlc.table->last = ((RK_U32) j + 1);
                break;
            }
        }
    }

    return (JPEGDEC_OK);
    FUN_TEST("Exit");
}

JpegDecRet jpegd_decode_quant_tables(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    RK_U32 t, tmp, i;
    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    StreamStorage *pStream = &(pSyntax->stream);

    pSyntax->quant.Lq = jpegd_get_two_bytes(pStream);

    /* check if there is enough data for tables */
    if (((pStream->readBits / 8) + pSyntax->quant.Lq) > pStream->streamLength) {
        JPEGD_ERROR_LOG("no enough data for tables");
        return (JPEGDEC_STRM_ERROR);
    }

    t = 4;

    while (t < pSyntax->quant.Lq) {
        /* Tq value selects what table the components use */

        /* read tables and write to decData->quant */
        tmp = jpegd_get_byte(pStream);
        t++;
        /* supporting only 8 bits / sample */
        if ((tmp >> 4) != 0) {
            JPEGD_ERROR_LOG("Pq(%d) is not supported!", (tmp >> 4));
            return (JPEGDEC_UNSUPPORTED);
        }

        tmp &= 0xF; /* Tq */
        /* set the quantisation table pointer */

        if (tmp == 0) {
            JPEGD_VERBOSE_LOG("qtable0\n");
            pSyntax->quant.table = pSyntax->quant.table0;
        } else if (tmp == 1) {
            JPEGD_VERBOSE_LOG("qtable1\n");
            pSyntax->quant.table = pSyntax->quant.table1;
        } else if (tmp == 2) {
            JPEGD_VERBOSE_LOG("qtable2\n");
            pSyntax->quant.table = pSyntax->quant.table2;
        } else if (tmp == 3) {
            JPEGD_VERBOSE_LOG("qtable3\n");
            pSyntax->quant.table = pSyntax->quant.table3;
        } else {
            JPEGD_ERROR_LOG("Tq(%d) is not supported!", tmp);
            return (JPEGDEC_UNSUPPORTED);
        }
        for (i = 0; i < 64; i++) {
            pSyntax->quant.table[i] = jpegd_get_byte(pStream);
            t++;
        }
    }

    return (MPP_OK);
    FUN_TEST("Exit");
}

JpegDecRet jpegd_default_huffman_tables(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }
    JpegSyntaxParam *pSyntax = pCtx->pSyntax;

    /* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
    /* IMPORTANT: these are only valid for 8-bit data precision! */
    uint8_t ff_mjpeg_bits_dc_luminance[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t ff_mjpeg_val_dc[12] =       //luminance and chrominance all use it
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    uint8_t ff_mjpeg_bits_dc_chrominance[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };

    uint8_t ff_mjpeg_bits_ac_luminance[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
    uint8_t ff_mjpeg_val_ac_luminance[] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    };

    uint8_t ff_mjpeg_bits_ac_chrominance[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };

    uint8_t ff_mjpeg_val_ac_chrominance[] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
        0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
        0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
        0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    };

    RK_U32 i, len, tmp, k;
    RK_S32 j;
    uint8_t *bitstable[4] = {ff_mjpeg_bits_dc_luminance, ff_mjpeg_bits_ac_luminance, ff_mjpeg_bits_dc_chrominance, ff_mjpeg_bits_ac_chrominance};
    uint8_t *valtable[4] = {ff_mjpeg_val_dc, ff_mjpeg_val_ac_luminance, ff_mjpeg_val_dc, ff_mjpeg_val_ac_chrominance};
    uint8_t *bitstmp;
    uint8_t *valtmp;

    for (k = 0; k < 4; k++) {
        bitstmp = bitstable[k];
        valtmp = valtable[k];

        if (k == 0)
            pSyntax->vlc.table = &(pSyntax->vlc.dcTable0);
        else if (k == 1)
            pSyntax->vlc.table = &(pSyntax->vlc.acTable0);
        else if (k == 2)
            pSyntax->vlc.table = &(pSyntax->vlc.dcTable1);
        else
            pSyntax->vlc.table = &(pSyntax->vlc.acTable1);

        tmp = 0;
        /* read in the values of list BITS */
        for (i = 0; i < 16; i++) {
            tmp += pSyntax->vlc.table->bits[i] = bitstmp[i + 1];
            len++;
        }
        /* allocate memory for HUFFVALs */
        if (pSyntax->vlc.table->vals != NULL) {
            /* free previously reserved table */
            mpp_free(pSyntax->vlc.table->vals);
        }

        pSyntax->vlc.table->vals = (RK_U32 *) mpp_calloc(RK_U32, tmp);

        /* set the table length */
        pSyntax->vlc.table->tableLength = tmp;
        /* read in the HUFFVALs */
        for (i = 0; i < tmp; i++) {
            pSyntax->vlc.table->vals[i] = valtmp[i];
            len++;
        }
        /* first and last lengths */
        for (i = 0; i < 16; i++) {
            if (pSyntax->vlc.table->bits[i] != 0) {
                pSyntax->vlc.table->start = i;
                break;
            }
        }
        for (j = 15; j >= 0; j--) {
            if (pSyntax->vlc.table->bits[j] != 0) {
                pSyntax->vlc.table->last = ((RK_U32) j + 1);
                break;
            }
        }
    }

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_parser_split_frame(RK_U8 *src, RK_U32 src_size, RK_U8 *dst, RK_U32 *dst_size)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    if (NULL == src || NULL == dst || src_size <= 0) {
        JPEGD_ERROR_LOG("NULL pointer or wrong src_size(%d)", src_size);
        return MPP_ERR_NULL_PTR;
    }
    RK_U8 *end;
    RK_U8 *tmp;
    RK_U32 str_size = (src_size + 255) & (~255);
    end = dst + src_size;

    if (src[6] == 0x41 && src[7] == 0x56 && src[8] == 0x49 && src[9] == 0x31) {
        //distinguish 310 from 210 camera
        RK_U32     i;
        RK_U32 copy_len = 0;
        tmp = src;
        JPEGD_INFO_LOG("distinguish 310 from 210 camera");

        for (i = 0; i < src_size - 4; i++) {
            if (tmp[i] == 0xff) {
                if (tmp[i + 1] == 0x00 && tmp[i + 2] == 0xff && ((tmp[i + 3] & 0xf0) == 0xd0))
                    i += 2;
            }
            *dst++ = tmp[i];
            copy_len++;
        }
        for (; i < src_size; i++) {
            *dst++ = tmp[i];
            copy_len++;
        }
        if (copy_len < src_size)
            memset(dst, 0, src_size - copy_len);
        *dst_size = copy_len;
    } else {
        memcpy(dst, src, src_size);
        memset(dst + src_size, 0, str_size - src_size);
        *dst_size = src_size;
    }

    /* NOTE: hardware bug, need to remove tailing FF 00 before FF D9 end flag */
    if (end[-1] == 0xD9 && end[-2] == 0xFF) {
        end -= 2;

        do {
            if (end[-1] == 0xFF) {
                end--;
                continue;
            }
            if (end[-1] == 0x00 && end [-2] == 0xFF) {
                end -= 2;
                continue;
            }
            break;
        } while (1);

        JPEGD_INFO_LOG("remove tailing FF 00 before FF D9 end flag.");
        end[0] = 0xff;
        end[1] = 0xD9;
    }

    FUN_TEST("Exit");
    return ret;
}

MPP_RET jpegd_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;
    MppPacket input_packet = JpegParserCtx->input_packet;
    RK_U32 copy_length = 0;
    void *base = mpp_packet_get_pos(pkt);
    RK_U8 *pos = base;
    RK_U32 pkt_length = (RK_U32)mpp_packet_get_length(pkt);
    RK_U32 eos = (pkt_length) ? (mpp_packet_get_eos(pkt)) : (1);

    JpegParserCtx->pts = mpp_packet_get_pts(pkt);

    task->valid = 0;
    task->flags.eos = eos;
    JpegParserCtx->eos = eos;

    JPEGD_INFO_LOG("pkt_length %d eos %d\n", pkt_length, eos);

    if (!pkt_length) {
        JPEGD_INFO_LOG("it is end of stream.");
        return ret;
    }

    if (pkt_length > JpegParserCtx->bufferSize) {
        JPEGD_INFO_LOG("Huge Frame(%d Bytes)! bufferSize:%d", pkt_length, JpegParserCtx->bufferSize);
        mpp_free(JpegParserCtx->recv_buffer);
        JpegParserCtx->recv_buffer = NULL;

        JpegParserCtx->recv_buffer = mpp_calloc(RK_U8, pkt_length + 1024);
        if (NULL == JpegParserCtx->recv_buffer) {
            JPEGD_ERROR_LOG("no memory!");
            return MPP_ERR_NOMEM;
        }

        JpegParserCtx->bufferSize = pkt_length + 1024;
    }

    jpegd_parser_split_frame(base, pkt_length, JpegParserCtx->recv_buffer, &copy_length);

    pos += pkt_length;
    mpp_packet_set_pos(pkt, pos);
    if (copy_length != pkt_length) {
        JPEGD_INFO_LOG("there seems to be something wrong with split_frame. pkt_length:%d, copy_length:%d", pkt_length, copy_length);
    }

    /* debug information */
    if (JpegParserCtx->parser_debug_enable && JpegParserCtx->input_jpeg_count < 3) {
        static FILE *jpg_file;
        static char name[32];

        snprintf(name, sizeof(name), "/data/input%02d.jpg", JpegParserCtx->input_jpeg_count);
        jpg_file = fopen(name, "wb+");
        if (jpg_file) {
            JPEGD_INFO_LOG("input jpeg(%d Bytes) saving to %s\n", pkt_length, name);
            fwrite(base, pkt_length, 1, jpg_file);
            fclose(jpg_file);
            JpegParserCtx->input_jpeg_count++;
        }
    }

    mpp_packet_set_data(input_packet, JpegParserCtx->recv_buffer);
    mpp_packet_set_size(input_packet, pkt_length);
    mpp_packet_set_length(input_packet, pkt_length);

    JpegParserCtx->streamLength = pkt_length;
    task->input_packet = input_packet;
    task->valid = 1;
    JPEGD_VERBOSE_LOG("input_packet:%p, recv_buffer:%p, pkt_length:%d", input_packet,
                      JpegParserCtx->recv_buffer, pkt_length);

    FUN_TEST("Exit");
    return ret;
}

MPP_RET jpegd_read_decode_parameters(JpegParserContext *ctx, StreamStorage *pStream)
{
    FUN_TEST("Enter");
    if (NULL == ctx || NULL == pStream) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    JpegParserContext *pCtx = ctx;
    JpegDecImageInfo *pImageInfo = (JpegDecImageInfo *) & (pCtx->pSyntax->imageInfo);
    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    RK_U32 Nf = 0, Ns = 0, NsThumb = 0;
    RK_U32 i, j = 0, init = 0, initThumb = 0;
    RK_U32 H[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 V[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 Htn[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 Vtn[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 Hmax = 0, Vmax = 0, headerLength = 0;
    RK_U32 currentByte = 0, currentBytes = 0;
    RK_U32 appLength = 0, appBits = 0;
    RK_U32 thumbnail = 0, errorCode = 0;

    /* reset sampling factors */
    for (i = 0; i < MAX_NUMBER_OF_COMPONENTS; i++) {
        H[i] = 0;
        V[i] = 0;
        Htn[i] = 0;
        Vtn[i] = 0;
    }

    /* Read decoding parameters */
    for (pStream->readBits = 0; (pStream->readBits / 8) < pStream->streamLength; pStream->readBits++) {
        /* Look for marker prefix byte from stream */
        if ((currentByte == 0xFF) || jpegd_get_byte(pStream) == 0xFF) {
            currentByte = jpegd_get_byte(pStream);

            switch (currentByte) { /* switch to certain header decoding */
            case SOF0: /* baseline marker */
            case SOF2: { /* progresive marker */
                JPEGD_VERBOSE_LOG("SOF, currentByte:0x%x", currentByte);
                if (currentByte == SOF0) {
                    pImageInfo->codingMode = pSyntax->info.operationType = JPEGDEC_BASELINE;
                } else {
                    pImageInfo->codingMode = pSyntax->info.operationType = JPEGDEC_PROGRESSIVE;
                }

                /* Frame header */
                i++;
                Hmax = 0;
                Vmax = 0;

                /* SOF0/SOF2 length */
                headerLength = jpegd_get_two_bytes(pStream);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    errorCode = 1;
                    break;
                }

                /* Sample precision (only 8 bits/sample supported) */
                currentByte = jpegd_get_byte(pStream);
                if (currentByte != 8) {
                    JPEGD_ERROR_LOG("Sample precision");
                    return (JPEGDEC_UNSUPPORTED);
                }

                /* Number of Lines */
                pImageInfo->outputHeight = jpegd_get_two_bytes(pStream);
                pImageInfo->displayHeight = pImageInfo->outputHeight;
                if (pImageInfo->outputHeight < 1) {
                    JPEGD_ERROR_LOG("pImageInfo->outputHeight(%d) Unsupported", pImageInfo->outputHeight);
                    return (JPEGDEC_UNSUPPORTED);
                }

                /* round up to next multiple-of-16 */
                pImageInfo->outputHeight += 0xf;
                pImageInfo->outputHeight &= ~(0xf);
                pSyntax->frame.hwY = pImageInfo->outputHeight;

                /* Number of Samples per Line */
                pImageInfo->outputWidth = jpegd_get_two_bytes(pStream);
                pImageInfo->displayWidth = pImageInfo->outputWidth;
                if (pImageInfo->outputWidth < 1) {
                    JPEGD_ERROR_LOG("pImageInfo->outputWidth(%d) Unsupported", pImageInfo->outputWidth);
                    return (JPEGDEC_UNSUPPORTED);
                }
                pImageInfo->outputWidth += 0xf;
                pImageInfo->outputWidth &= ~(0xf);
                pSyntax->frame.hwX = pImageInfo->outputWidth;

                /* check for minimum and maximum dimensions */
                if (pImageInfo->outputWidth < pCtx->minSupportedWidth ||
                    pImageInfo->outputHeight < pCtx->minSupportedHeight ||
                    pImageInfo->outputWidth > pCtx->maxSupportedWidth ||
                    pImageInfo->outputHeight > pCtx->maxSupportedHeight ||
                    (pImageInfo->outputWidth * pImageInfo->outputHeight) > pCtx->maxSupportedPixelAmount) {
                    JPEGD_ERROR_LOG("Unsupported size(%d*%d)", pImageInfo->outputWidth, pImageInfo->outputHeight);
                    return (JPEGDEC_UNSUPPORTED);
                }

                /* Number of Image Components per Frame */
                Nf = jpegd_get_byte(pStream);
                if (Nf != 3 && Nf != 1) {
                    JPEGD_ERROR_LOG("Number of Image Components per Frame: %d", Nf);
                    return (JPEGDEC_UNSUPPORTED);
                }
                for (j = 0; j < Nf; j++) {
                    if (jpegd_flush_bits(pStream, 8) == STRM_ERROR) { /* jump over component identifier */
                        errorCode = 1;
                        break;
                    }

                    currentByte = jpegd_get_byte(pStream);
                    H[j] = (currentByte >> 4);   /* Horizontal sampling factor */
                    V[j] = (currentByte & 0xF);  /* Vertical sampling factor */

                    if (jpegd_flush_bits(pStream, 8) == STRM_ERROR) { /* jump over Tq */
                        errorCode = 1;
                        break;
                    }

                    if (H[j] > Hmax)
                        Hmax = H[j];
                    if (V[j] > Vmax)
                        Vmax = V[j];
                }
                if (Hmax == 0 || Vmax == 0) {
                    JPEGD_ERROR_LOG("Hmax(%d), Vmax(%d)", Hmax, Vmax);
                    return (JPEGDEC_UNSUPPORTED);
                }

                /* check format */
                if (H[0] == 2 && V[0] == 2 &&
                    H[1] == 1 && V[1] == 1 && H[2] == 1 && V[2] == 1) {
                    pImageInfo->outputFormat = MPP_FMT_YUV420SP;
                    pSyntax->frame.numMcuInRow = (pSyntax->frame.hwX / 16);
                    pSyntax->frame.numMcuInFrame = ((pSyntax->frame.hwX * pSyntax->frame.hwY) / 256);
                } else if (H[0] == 2 && V[0] == 1 && H[1] == 1 && V[1] == 1 && H[2] == 1 && V[2] == 1) {
                    pImageInfo->outputFormat = MPP_FMT_YUV422SP;
                    pSyntax->frame.numMcuInRow = (pSyntax->frame.hwX / 16);
                    pSyntax->frame.numMcuInFrame = ((pSyntax->frame.hwX * pSyntax->frame.hwY) / 128);
                } else if (H[0] == 1 && V[0] == 2 &&
                           H[1] == 1 && V[1] == 1 && H[2] == 1 && V[2] == 1) {
                    pImageInfo->outputFormat = JPEGDEC_YCbCr440;
                    pSyntax->frame.numMcuInRow = (pSyntax->frame.hwX / 8);
                    pSyntax->frame.numMcuInFrame = ((pSyntax->frame.hwX * pSyntax->frame.hwY) / 128);
                } else if (H[0] == 1 && V[0] == 1 &&
                           H[1] == 0 && V[1] == 0 && H[2] == 0 && V[2] == 0) {
                    pImageInfo->outputFormat = JPEGDEC_YCbCr400;
                    pSyntax->frame.numMcuInRow = (pSyntax->frame.hwX / 8);
                    pSyntax->frame.numMcuInFrame = ((pSyntax->frame.hwX * pSyntax->frame.hwY) / 64);
                } else if (pCtx->extensionsSupported &&
                           H[0] == 4 && V[0] == 1 &&
                           H[1] == 1 && V[1] == 1 && H[2] == 1 && V[2] == 1) {
                    /* YUV411 output has to be 32 pixel multiple */
                    if (pImageInfo->outputWidth & 0x1F) {
                        pImageInfo->outputWidth += 16;
                        pSyntax->frame.hwX = pImageInfo->outputWidth;
                    }

                    /* check for maximum dimensions */
                    if (pImageInfo->outputWidth > pCtx->maxSupportedWidth ||
                        (pImageInfo->outputWidth * pImageInfo->outputHeight) > pCtx->maxSupportedPixelAmount) {
                        JPEGD_ERROR_LOG("Unsupported size(%d*%d)", pImageInfo->outputWidth, pImageInfo->outputHeight);
                        return (JPEGDEC_UNSUPPORTED);
                    }

                    pImageInfo->outputFormat = JPEGDEC_YCbCr411_SEMIPLANAR;
                    pSyntax->frame.numMcuInRow = (pSyntax->frame.hwX / 32);
                    pSyntax->frame.numMcuInFrame = ((pSyntax->frame.hwX * pSyntax->frame.hwY) / 256);
                } else if (pCtx->extensionsSupported &&
                           H[0] == 1 && V[0] == 1 &&
                           H[1] == 1 && V[1] == 1 && H[2] == 1 && V[2] == 1) {
                    pImageInfo->outputFormat = JPEGDEC_YCbCr444_SEMIPLANAR;
                    pSyntax->frame.numMcuInRow = (pSyntax->frame.hwX / 8);
                    pSyntax->frame.numMcuInFrame = ((pSyntax->frame.hwX * pSyntax->frame.hwY) / 64);
                } else {
                    JPEGD_ERROR_LOG("Unsupported YCbCr format");
                    return (JPEGDEC_UNSUPPORTED);
                }

                /* restore output format */
                pSyntax->info.yCbCrMode = pSyntax->info.getInfoYCbCrMode = pImageInfo->outputFormat;
                break;
            }
            case SOS: {
                JPEGD_VERBOSE_LOG("SOS, currentByte:0x%x", currentByte);
                /* SOS length */
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("Length of <Start of Scan> is %d", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }

                /* check if interleaved or non-ibnterleaved */
                Ns = jpegd_get_byte(pStream);
                if (Ns == MIN_NUMBER_OF_COMPONENTS &&
                    pImageInfo->outputFormat != JPEGDEC_YCbCr400 && pImageInfo->codingMode == JPEGDEC_BASELINE) {
                    pImageInfo->codingMode = pSyntax->info.operationType = JPEGDEC_NONINTERLEAVED;
                }

                /* jump over SOS header */
                if (headerLength != 0) {
                    pStream->readBits += ((headerLength * 8) - 16);
                    pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                }

                if ((pStream->readBits + 8) < (8 * pStream->streamLength)) {
                    pSyntax->info.init = 1;
                    init = 1;
                } else {
                    JPEGD_ERROR_LOG("Needs to increase input buffer");
                    return (JPEGDEC_INCREASE_INPUT_BUFFER);
                }
                break;
            }
            case DQT: {
                JPEGD_VERBOSE_LOG("DQT, currentByte:0x%x", currentByte);
                /* DQT length */
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("Length of Define Quantization Table is %d", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }

                /* jump over DQT header */
                if (headerLength != 0) {
                    pStream->readBits += ((headerLength * 8) - 16);
                    pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                }
                break;
            }
            case DHT: {
                JPEGD_VERBOSE_LOG("DHT, currentByte:0x%x", currentByte);
                /* DHT length */
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("Length of Define Huffman Table is %d", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }
                /* jump over DHT header */
                if (headerLength != 0) {
                    pStream->readBits += ((headerLength * 8) - 16);
                    pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                }
                break;
            }
            case DRI: {
                JPEGD_VERBOSE_LOG("DRI, currentByte:0x%x", currentByte);
                /* DRI length */
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("Length of Define Restart Interval(must be 4 Bytes) is %d", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }

                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("Restart Interval:%d", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    /* may optimize one day */
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }
                pSyntax->frame.Ri = headerLength;
                break;
            }
            case APP0: { /* application segments */
                JPEGD_VERBOSE_LOG("APP0, currentByte:0x%x", currentByte);
                /* reset */
                appBits = 0;
                appLength = 0;
                pStream->appnFlag = 0;

                /* APP0 length */
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("headerLength:%d", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }

                appLength = headerLength;
                if (appLength < 16) {
                    pStream->appnFlag = 1;
                    if (jpegd_flush_bits(pStream, ((appLength * 8) - 16)) == STRM_ERROR) {
                        JPEGD_ERROR_LOG("flush bits failed");
                        errorCode = 1;
                        break;
                    }
                    break;
                }
                appBits += 16;

                /* check identifier: 0x4A 0x46 0x49 0x46 0x00 */
                currentBytes = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("currentBytes:%x", currentBytes);
                appBits += 16;
                if (currentBytes != 0x4A46) {
                    pStream->appnFlag = 1;
                    if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                        JPEGD_ERROR_LOG("flush bits failed");
                        errorCode = 1;
                        break;
                    }
                    break;
                }

                currentBytes = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("currentBytes:%x", currentBytes);
                appBits += 16;
                if (currentBytes != 0x4946 && currentBytes != 0x5858) {
                    pStream->appnFlag = 1;
                    if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                        JPEGD_ERROR_LOG("flush bits failed");
                        errorCode = 1;
                        break;
                    }
                    break;
                }

                /* APP0 Extended */
                if (currentBytes == 0x5858) {
                    JPEGD_VERBOSE_LOG("APP0 Extended");
                    thumbnail = 1;
                }
                currentByte = jpegd_get_byte(pStream);
                JPEGD_VERBOSE_LOG("currentBytes:%x", currentBytes);
                appBits += 8;
                if (currentByte != 0x00) {
                    pStream->appnFlag = 1;
                    if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                        JPEGD_ERROR_LOG("flush bits failed");
                        errorCode = 1;
                        break;
                    }
                    pStream->appnFlag = 0;
                    break;
                }

                /* APP0 Extended thumb type */
                if (thumbnail) {
                    /* extension code */
                    currentByte = jpegd_get_byte(pStream);
                    JPEGD_VERBOSE_LOG("APP0 Extended thumb type, currentBytes:%x", currentBytes);
                    if (currentByte == JPEGDEC_THUMBNAIL_JPEG) {
                        pImageInfo->thumbnailType = JPEGDEC_THUMBNAIL_JPEG;
                        appBits += 8;
                        pStream->appnFlag = 1;

                        /* check thumbnail data */
                        Hmax = 0;
                        Vmax = 0;

                        /* Read decoding parameters */
                        for (; (pStream->readBits / 8) < pStream->streamLength; pStream->readBits++) {
                            appBits += 8; /* Look for marker prefix byte from stream */
                            if (jpegd_get_byte(pStream) == 0xFF) {
                                appBits += 8;

                                currentByte = jpegd_get_byte(pStream);
                                switch (currentByte) { /* switch to certain header decoding */
                                case SOF0: /* baseline marker */
                                case SOF2: { /* progresive marker */
                                    if (currentByte == SOF0)
                                        pImageInfo->codingModeThumb = pSyntax->info.operationTypeThumb = JPEGDEC_BASELINE;
                                    else
                                        pImageInfo->codingModeThumb = pSyntax->info.operationTypeThumb = JPEGDEC_PROGRESSIVE;

                                    /* Frame header */
                                    i++;

                                    /* jump over Lf field */
                                    if (jpegd_flush_bits(pStream, 16) == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    appBits += 16;

                                    /* Sample precision (only 8 bits/sample supported) */
                                    currentByte = jpegd_get_byte(pStream);
                                    appBits += 8;
                                    if (currentByte != 8) {
                                        JPEGD_ERROR_LOG("Thumbnail Sample precision");
                                        return (JPEGDEC_UNSUPPORTED);
                                    }

                                    /* Number of Lines */
                                    pImageInfo->outputHeightThumb = jpegd_get_two_bytes(pStream);
                                    appBits += 16;
                                    pImageInfo->displayHeightThumb = pImageInfo->outputHeightThumb;
                                    if (pImageInfo->outputHeightThumb < 1) {
                                        JPEGD_ERROR_LOG("pImageInfo->outputHeightThumb unsupported");
                                        return (JPEGDEC_UNSUPPORTED);
                                    }

                                    /* round up to next multiple-of-16 */
                                    pImageInfo->outputHeightThumb += 0xf;
                                    pImageInfo->outputHeightThumb &= ~(0xf);

                                    /* Number of Samples per Line */
                                    pImageInfo->outputWidthThumb = jpegd_get_two_bytes(pStream);
                                    appBits += 16;
                                    pImageInfo->displayWidthThumb = pImageInfo->outputWidthThumb;
                                    if (pImageInfo->outputWidthThumb < 1) {
                                        JPEGD_ERROR_LOG("pImageInfo->outputWidthThumb unsupported");
                                        return (JPEGDEC_UNSUPPORTED);
                                    }

                                    pImageInfo->outputWidthThumb += 0xf;
                                    pImageInfo->outputWidthThumb &= ~(0xf);
                                    if (pImageInfo->outputWidthThumb < pCtx->minSupportedWidth ||
                                        pImageInfo->outputHeightThumb < pCtx->minSupportedHeight ||
                                        pImageInfo->outputWidthThumb > JPEGDEC_MAX_WIDTH_TN ||
                                        pImageInfo->outputHeightThumb > JPEGDEC_MAX_HEIGHT_TN) {
                                        JPEGD_ERROR_LOG("Thumbnail Unsupported size");
                                        return (JPEGDEC_UNSUPPORTED);
                                    }

                                    /* Number of Image Components per Frame */
                                    Nf = jpegd_get_byte(pStream);
                                    appBits += 8;
                                    if (Nf != 3 && Nf != 1) {
                                        JPEGD_ERROR_LOG("Thumbnail Number of Image Components per Frame");
                                        return (JPEGDEC_UNSUPPORTED);
                                    }
                                    for (j = 0; j < Nf; j++) {
                                        /* jump over component identifier */
                                        if (jpegd_flush_bits(pStream, 8) == STRM_ERROR) {
                                            errorCode = 1;
                                            break;
                                        }
                                        appBits += 8;

                                        currentByte = jpegd_get_byte(pStream);
                                        appBits += 8;
                                        Htn[j] = (currentByte >> 4);  /* Horizontal sampling factor */
                                        Vtn[j] = (currentByte & 0xF); /* Vertical sampling factor */

                                        if (jpegd_flush_bits(pStream, 8) == STRM_ERROR) { /* jump over Tq */
                                            errorCode = 1;
                                            break;
                                        }
                                        appBits += 8;

                                        if (Htn[j] > Hmax)
                                            Hmax = Htn[j];
                                        if (Vtn[j] > Vmax)
                                            Vmax = Vtn[j];
                                    }
                                    if (Hmax == 0 || Vmax == 0) {
                                        JPEGD_ERROR_LOG("Thumbnail Hmax == 0 || Vmax == 0");
                                        return (JPEGDEC_UNSUPPORTED);
                                    }

                                    /* check format */
                                    if (Htn[0] == 2 && Vtn[0] == 2 &&
                                        Htn[1] == 1 && Vtn[1] == 1 && Htn[2] == 1 && Vtn[2] == 1) {
                                        pImageInfo->outputFormatThumb = MPP_FMT_YUV420SP;
                                    } else if (Htn[0] == 2 && Vtn[0] == 1 &&
                                               Htn[1] == 1 && Vtn[1] == 1 && Htn[2] == 1 && Vtn[2] == 1) {
                                        pImageInfo->outputFormatThumb = MPP_FMT_YUV422SP;
                                    } else if (Htn[0] == 1 && Vtn[0] == 2 &&
                                               Htn[1] == 1 && Vtn[1] == 1 && Htn[2] == 1 && Vtn[2] == 1) {
                                        pImageInfo->outputFormatThumb = JPEGDEC_YCbCr440;
                                    } else if (Htn[0] == 1 && Vtn[0] == 1 &&
                                               Htn[1] == 0 && Vtn[1] == 0 && Htn[2] == 0 && Vtn[2] == 0) {
                                        pImageInfo->outputFormatThumb = JPEGDEC_YCbCr400;
                                    } else if (Htn[0] == 4 && Vtn[0] == 1 &&
                                               Htn[1] == 1 && Vtn[1] == 1 && Htn[2] == 1 && Vtn[2] == 1) {
                                        pImageInfo->outputFormatThumb = JPEGDEC_YCbCr411_SEMIPLANAR;
                                    } else if (Htn[0] == 1 && Vtn[0] == 1 &&
                                               Htn[1] == 1 && Vtn[1] == 1 && Htn[2] == 1 && Vtn[2] == 1) {
                                        pImageInfo->outputFormatThumb = JPEGDEC_YCbCr444_SEMIPLANAR;
                                    } else {
                                        JPEGD_ERROR_LOG("Thumbnail Unsupported YCbCr format");
                                        return (JPEGDEC_UNSUPPORTED);
                                    }
                                    pSyntax->info.initThumb = 1;
                                    initThumb = 1;
                                    break;
                                }
                                case SOS: {
                                    /* SOS length */
                                    headerLength = jpegd_get_two_bytes(pStream);
                                    if (headerLength == STRM_ERROR ||
                                        ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                                        errorCode = 1;
                                        break;
                                    }

                                    /* check if interleaved or non-ibnterleaved */
                                    NsThumb = jpegd_get_byte(pStream);
                                    if (NsThumb == MIN_NUMBER_OF_COMPONENTS &&
                                        pImageInfo->outputFormatThumb != JPEGDEC_YCbCr400 &&
                                        pImageInfo->codingModeThumb == JPEGDEC_BASELINE) {
                                        pImageInfo->codingModeThumb = pSyntax->info.operationTypeThumb = JPEGDEC_NONINTERLEAVED;
                                    }

                                    /* jump over SOS header */
                                    if (headerLength != 0) {
                                        pStream->readBits += ((headerLength * 8) - 16);
                                        pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                                    }

                                    if ((pStream->readBits + 8) < (8 * pStream->streamLength)) {
                                        pSyntax->info.init = 1;
                                        init = 1;
                                    } else {
                                        JPEGD_ERROR_LOG("Needs to increase input buffer");
                                        return (JPEGDEC_INCREASE_INPUT_BUFFER);
                                    }
                                    break;
                                }
                                case DQT: {
                                    /* DQT length */
                                    headerLength = jpegd_get_two_bytes(pStream);
                                    if (headerLength == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    /* jump over DQT header */
                                    if (headerLength != 0) {
                                        pStream->readBits += ((headerLength * 8) - 16);
                                        pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                                    }
                                    appBits += (headerLength * 8);
                                    break;
                                }
                                case DHT: {
                                    /* DHT length */
                                    headerLength = jpegd_get_two_bytes(pStream);
                                    if (headerLength == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    /* jump over DHT header */
                                    if (headerLength != 0) {
                                        pStream->readBits += ((headerLength * 8) - 16);
                                        pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                                    }
                                    appBits += (headerLength * 8);
                                    break;
                                }
                                case DRI: {
                                    /* DRI length */
                                    headerLength = jpegd_get_two_bytes(pStream);
                                    if (headerLength == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    /* jump over DRI header */
                                    if (headerLength != 0) {
                                        pStream->readBits += ((headerLength * 8) - 16);
                                        pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                                    }
                                    appBits += (headerLength * 8);
                                    break;
                                }
                                case APP0:
                                case APP1:
                                case APP2:
                                case APP3:
                                case APP4:
                                case APP5:
                                case APP6:
                                case APP7:
                                case APP8:
                                case APP9:
                                case APP10:
                                case APP11:
                                case APP12:
                                case APP13:
                                case APP14:
                                case APP15: {
                                    /* APPn length */
                                    headerLength = jpegd_get_two_bytes(pStream);
                                    if (headerLength == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    /* jump over APPn header */
                                    if (headerLength != 0) {
                                        pStream->readBits += ((headerLength * 8) - 16);
                                        pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                                    }
                                    appBits += (headerLength * 8);
                                    break;
                                }
                                case DNL: {
                                    /* DNL length */
                                    headerLength = jpegd_get_two_bytes(pStream);
                                    if (headerLength == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    /* jump over DNL header */
                                    if (headerLength != 0) {
                                        pStream->readBits += ((headerLength * 8) - 16);
                                        pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                                    }
                                    appBits += (headerLength * 8);
                                    break;
                                }
                                case COM: {
                                    /* COM length */
                                    headerLength = jpegd_get_two_bytes(pStream);
                                    if (headerLength == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    /* jump over COM header */
                                    if (headerLength != 0) {
                                        pStream->readBits += ((headerLength * 8) - 16);
                                        pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                                    }
                                    appBits += (headerLength * 8);
                                    break;
                                }
                                case SOF1: /* unsupported coding styles */
                                case SOF3:
                                case SOF5:
                                case SOF6:
                                case SOF7:
                                case SOF9:
                                case SOF10:
                                case SOF11:
                                case SOF13:
                                case SOF14:
                                case SOF15:
                                case DAC:
                                case DHP: {
                                    JPEGD_ERROR_LOG("Unsupported coding styles");
                                    return (JPEGDEC_UNSUPPORTED);
                                }
                                default:
                                    break;
                                }

                                if (pSyntax->info.initThumb && initThumb) {
                                    /* flush the rest of thumbnail data */
                                    if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                                        errorCode = 1;
                                        break;
                                    }
                                    pStream->appnFlag = 0;
                                    break;
                                }
                            } else {
                                if (!pSyntax->info.initThumb && pCtx->bufferSize)
                                    return (JPEGDEC_INCREASE_INPUT_BUFFER);
                                else
                                    return (JPEGDEC_STRM_ERROR);
                            }
                        }
                        break;
                    } else {
                        appBits += 8;
                        pImageInfo->thumbnailType = JPEGDEC_THUMBNAIL_NOT_SUPPORTED_FORMAT;
                        pStream->appnFlag = 1;
                        if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                            errorCode = 1;
                            break;
                        }
                        pStream->appnFlag = 0;
                        break;
                    }
                } else {
                    /* version */
                    pImageInfo->version = jpegd_get_two_bytes(pStream);
                    JPEGD_VERBOSE_LOG("APP0, version:%x", pImageInfo->version);
                    appBits += 16;

                    /* units */
                    currentByte = jpegd_get_byte(pStream);
                    if (currentByte == 0) {
                        pImageInfo->units = JPEGDEC_NO_UNITS;
                    } else if (currentByte == 1) {
                        pImageInfo->units = JPEGDEC_DOTS_PER_INCH;
                    } else if (currentByte == 2) {
                        pImageInfo->units = JPEGDEC_DOTS_PER_CM;
                    }
                    appBits += 8;

                    /* Xdensity */
                    pImageInfo->xDensity = jpegd_get_two_bytes(pStream);
                    appBits += 16;

                    /* Ydensity */
                    pImageInfo->yDensity = jpegd_get_two_bytes(pStream);
                    appBits += 16;

                    /* jump over rest of header data */
                    pStream->appnFlag = 1;
                    if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                        errorCode = 1;
                        break;
                    }
                    pStream->appnFlag = 0;
                    break;
                }
            }
            case APP1:
            case APP2:
            case APP3:
            case APP4:
            case APP5:
            case APP6:
            case APP7:
            case APP8:
            case APP9:
            case APP10:
            case APP11:
            case APP12:
            case APP13:
            case APP14:
            case APP15: {
                JPEGD_VERBOSE_LOG("APPn, currentByte:0x%x", currentByte);
                /* APPn length */
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("APPn length, headerLength:%x", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }
                /* jump over APPn header */
                if (headerLength != 0) {
                    pStream->readBits += ((headerLength * 8) - 16);
                    pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                }
                break;
            }
            case DNL: {
                JPEGD_VERBOSE_LOG("DNL, currentByte:0x%x", currentByte);
                /* DNL length */
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("DNL, headerLength:%x", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }
                /* jump over DNL header */
                if (headerLength != 0) {
                    pStream->readBits += ((headerLength * 8) - 16);
                    pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                }
                break;
            }
            case COM: {
                JPEGD_VERBOSE_LOG("COM, currentByte:0x%x", currentByte);
                headerLength = jpegd_get_two_bytes(pStream);
                JPEGD_VERBOSE_LOG("COM, headerLength:%x", headerLength);
                if (headerLength == STRM_ERROR ||
                    ((pStream->readBits + ((headerLength * 8) - 16)) > (8 * pStream->streamLength))) {
                    JPEGD_ERROR_LOG("readBits:%d, headerLength:%d, streamLength:%d", pStream->readBits, headerLength, pStream->streamLength);
                    errorCode = 1;
                    break;
                }
                /* jump over COM header */
                if (headerLength != 0) {
                    pStream->readBits += ((headerLength * 8) - 16);
                    pStream->pCurrPos += (((headerLength * 8) - 16) / 8);
                }
                break;
            }
            case SOF1:   /* unsupported coding styles */
            case SOF3:
            case SOF5:
            case SOF6:
            case SOF7:
            case SOF9:
            case SOF10:
            case SOF11:
            case SOF13:
            case SOF14:
            case SOF15:
            case DAC:
            case DHP: {
                JPEGD_ERROR_LOG("SOF, currentByte:0x%x", currentByte);
                JPEGD_ERROR_LOG("Unsupported coding styles");
                return (JPEGDEC_UNSUPPORTED);
            }
            default:
                break;
            }
            if (pSyntax->info.init && init)
                break;

            if (errorCode) {
                if (pCtx->bufferSize) {
                    JPEGD_ERROR_LOG("get image info failed!");
                    return (JPEGDEC_INCREASE_INPUT_BUFFER);
                } else {
                    JPEGD_ERROR_LOG("Stream error");
                    return (JPEGDEC_STRM_ERROR);
                }
            }
        } else {
            JPEGD_ERROR_LOG("Could not get marker");
            if (!pSyntax->info.init)
                return (JPEGDEC_INCREASE_INPUT_BUFFER);
            else
                return (JPEGDEC_STRM_ERROR);
        }
    }

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_get_image_info(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    StreamStorage stream;

    if (pCtx->streamLength < 1) {
        JPEGD_ERROR_LOG("streamLength:%d", pCtx->streamLength);
        return MPP_ERR_VALUE;
    }

    if ((pCtx->streamLength > DEC_MAX_STREAM) &&
        (pCtx->bufferSize < JPEGDEC_MIN_BUFFER || pCtx->bufferSize > JPEGDEC_MAX_BUFFER)) {
        JPEGD_ERROR_LOG("bufferSize = %d,streamLength = %d\n", pCtx->bufferSize, pCtx->streamLength);
        return MPP_ERR_VALUE;
    }

    if (pCtx->bufferSize && (pCtx->bufferSize < JPEGDEC_MIN_BUFFER ||
                             pCtx->bufferSize > JPEGDEC_MAX_BUFFER)) {
        JPEGD_ERROR_LOG("bufferSize = %d\n", pCtx->bufferSize);
        return MPP_ERR_VALUE;
    }

    pSyntax->imageInfo.thumbnailType = JPEGDEC_NO_THUMBNAIL;

    /* utils initialization */
    stream.bitPosInByte = 0;
    stream.pCurrPos = (RK_U8 *)mpp_packet_get_data(pCtx->input_packet);
    stream.pStartOfStream = (RK_U8 *)mpp_packet_get_data(pCtx->input_packet);
    stream.streamLength = (RK_U32)mpp_packet_get_size(pCtx->input_packet);
    stream.readBits = 0;
    stream.appnFlag = 0;

    ret = jpegd_read_decode_parameters(pCtx, &stream);
    if (ret != MPP_OK) {
        JPEGD_ERROR_LOG("read decode parameters failed, ret:%d", ret);
        return ret;
    }

    FUN_TEST("Exit");
    return ret;
}

MPP_RET jpegd_decode_frame_impl(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    RK_U32 i = 0;
    RK_U32 currentByte = 0;
    RK_U32 currentBytes = 0;
    RK_U32 appLength = 0;
    RK_U32 appBits = 0;
    JpegDecRet retCode;
    RK_U32 findhufftable = 0;

    JpegSyntaxParam *pSyntax = pCtx->pSyntax;
    StreamStorage *pStream = (StreamStorage *) & (pSyntax->stream);

    do {
        /* if slice mode/slice done return to hw handling */
        if (pSyntax->image.headerReady && pSyntax->info.SliceReadyForPause)
            break;

        /* Look for marker prefix byte from stream */
        if ((currentByte == 0xFF) || (jpegd_get_byte(pStream) == 0xFF)) {
            currentByte = jpegd_get_byte(pStream);

            /* switch to certain header decoding */
            switch (currentByte) {
            case 0x00: {
                JPEGD_VERBOSE_LOG("currentByte:0x00");
                break;
            }
            case SOF0:
            case SOF2: {
                JPEGD_VERBOSE_LOG("SOF, currentByte:0x%x", currentByte);
                /* Baseline/Progressive */
                pSyntax->frame.codingType = currentByte;
                /* Set operation type */
                if (pSyntax->frame.codingType == SOF0)
                    pSyntax->info.operationType = JPEGDEC_BASELINE;
                else
                    pSyntax->info.operationType = JPEGDEC_PROGRESSIVE;

                retCode = jpegd_decode_frame_header(pCtx);
                if (retCode != JPEGDEC_OK) {
                    if (retCode == JPEGDEC_STRM_ERROR) {
                        JPEGD_ERROR_LOG("Stream Error");
                        return (retCode);
                    } else {
                        JPEGD_ERROR_LOG("Decode Frame Header Error");
                        return (retCode);
                    }
                }
                break;
            }
            case SOS: { /* Start of Scan */
                JPEGD_VERBOSE_LOG("SOS, currentByte:0x%x", currentByte);
                /* reset image ready */
                pSyntax->image.imageReady = 0;
                retCode = jpegd_decode_scan(pCtx);
                pSyntax->image.headerReady = 1;
                if (retCode != JPEGDEC_OK) {
                    if (retCode == JPEGDEC_STRM_ERROR) {
                        JPEGD_ERROR_LOG("Stream Error");
                        return (retCode);
                    } else {
                        JPEGD_ERROR_LOG("Decode Scan Error\n");
                        return (retCode);
                    }
                }

                if (pSyntax->stream.bitPosInByte) {
                    /* delete stuffing bits */
                    currentByte = (8 - pSyntax->stream.bitPosInByte);
                    if (jpegd_flush_bits(pStream, 8 - pSyntax->stream.bitPosInByte) == STRM_ERROR) {
                        JPEGD_ERROR_LOG("Stream Error");
                        return (JPEGDEC_STRM_ERROR);
                    }
                }
                JPEGD_VERBOSE_LOG("Stuffing bits deleted\n");
                break;
            }
            case DHT: { /* Start of Huffman tables */
                JPEGD_VERBOSE_LOG("DHT, currentByte:0x%x", currentByte);
                retCode = jpegd_decode_huffman_tables(pCtx);
                if (retCode != JPEGDEC_OK) {
                    if (retCode == JPEGDEC_STRM_ERROR) {
                        JPEGD_ERROR_LOG("Stream Error");
                        return (retCode);
                    } else {
                        JPEGD_ERROR_LOG("Decode Huffman Tables Error");
                        return (retCode);
                    }
                }
                findhufftable = 1;
                break;
            }
            case DQT: { /* start of Quantisation Tables */
                JPEGD_VERBOSE_LOG("DQT, currentByte:0x%x", currentByte);
                retCode = jpegd_decode_quant_tables(pCtx);
                if (retCode != JPEGDEC_OK) {
                    if (retCode == JPEGDEC_STRM_ERROR) {
                        JPEGD_ERROR_LOG("Stream Error");
                        return (retCode);
                    } else {
                        JPEGD_ERROR_LOG("Decode Quant Tables Error");
                        return (retCode);
                    }
                }
                break;
            }
            case SOI: { /* Start of Image */
                /* no actions needed, continue */
                break;
            }
            case EOI: { /* End of Image */
                JPEGD_VERBOSE_LOG("EOI, currentByte:0x%x", currentByte);
                if (pSyntax->image.imageReady) {
                    JPEGD_ERROR_LOG("EOI: OK\n");
                    return (JPEGDEC_FRAME_READY);
                } else {
                    JPEGD_ERROR_LOG("EOI: NOK\n");
                    return (JPEGDEC_ERROR);
                }
            }
            case DRI: { /* Define Restart Interval */
                JPEGD_VERBOSE_LOG("DRI, currentByte:0x%x", currentByte);
                currentBytes = jpegd_get_two_bytes(pStream);
                if (currentBytes == STRM_ERROR) {
                    JPEGD_ERROR_LOG("Read bits ");
                    return (JPEGDEC_STRM_ERROR);
                }
                pSyntax->frame.Ri = jpegd_get_two_bytes(pStream);
                break;
            }
            case RST0: /* Restart with modulo 8 count m */
            case RST1:
            case RST2:
            case RST3:
            case RST4:
            case RST5:
            case RST6:
            case RST7: {
                JPEGD_VERBOSE_LOG("RST, currentByte:0x%x", currentByte);
                /* initialisation of DC predictors to zero value !!! */
                for (i = 0; i < MAX_NUMBER_OF_COMPONENTS; i++) {
                    pSyntax->scan.pred[i] = 0;
                }
                break;
            }
            case DNL:  /* unsupported features */
            case SOF1:
            case SOF3:
            case SOF5:
            case SOF6:
            case SOF7:
            case SOF9:
            case SOF10:
            case SOF11:
            case SOF13:
            case SOF14:
            case SOF15:
            case DAC:
            case DHP:
            case TEM: {
                JPEGD_ERROR_LOG("Unsupported Features, currentByte:0x%x", currentByte);
                return (JPEGDEC_UNSUPPORTED);
            }
            case APP0: { /* application data & comments */
                JPEGD_VERBOSE_LOG("APP0, currentByte:0x%x", currentByte);
                /* APP0 Extended Thumbnail */
                if (pCtx->decImageType == JPEGDEC_THUMBNAIL) {
                    /* reset */
                    appBits = 0;
                    appLength = 0;

                    /* length */
                    appLength = jpegd_get_two_bytes(pStream);
                    appBits += 16;

                    /* check identifier */
                    currentBytes = jpegd_get_two_bytes(pStream);
                    appBits += 16;
                    if (currentBytes != 0x4A46) {
                        pSyntax->stream.appnFlag = 1;
                        if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                            JPEGD_ERROR_LOG("Stream Error");
                            return (JPEGDEC_STRM_ERROR);
                        }
                        pSyntax->stream.appnFlag = 0;
                        break;
                    }

                    currentBytes = jpegd_get_two_bytes(pStream);
                    appBits += 16;
                    if (currentBytes != 0x5858) {
                        pSyntax->stream.appnFlag = 1;
                        if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                            JPEGD_ERROR_LOG("Stream Error");
                            return (JPEGDEC_STRM_ERROR);
                        }
                        pSyntax->stream.appnFlag = 0;
                        break;
                    }

                    currentByte = jpegd_get_byte(pStream);
                    appBits += 8;
                    if (currentByte != 0x00) {
                        pSyntax->stream.appnFlag = 1;
                        if (jpegd_flush_bits(pStream, ((appLength * 8) - appBits)) == STRM_ERROR) {
                            JPEGD_ERROR_LOG("Stream Error");
                            return (JPEGDEC_STRM_ERROR);
                        }
                        pSyntax->stream.appnFlag = 0;
                        break;
                    }

                    /* extension code */
                    currentByte = jpegd_get_byte(pStream);
                    pSyntax->stream.appnFlag = 0;
                    if (currentByte != JPEGDEC_THUMBNAIL_JPEG) {
                        JPEGD_ERROR_LOG("thumbnail unsupported");
                        return (JPEGDEC_UNSUPPORTED);
                    }

                    /* thumbnail mode */
                    JPEGD_VERBOSE_LOG("Thumbnail data ok!");
                    pSyntax->stream.thumbnail = 1;
                    break;
                } else {
                    /* Flush unsupported thumbnail */
                    currentBytes = jpegd_get_two_bytes(pStream);
                    pSyntax->stream.appnFlag = 1;
                    if (jpegd_flush_bits(pStream, ((currentBytes - 2) * 8)) == STRM_ERROR) {
                        JPEGD_ERROR_LOG("Stream Error");
                        return (JPEGDEC_STRM_ERROR);
                    }
                    pSyntax->stream.appnFlag = 0;
                    break;
                }
            }
            case APP1:
            case APP2:
            case APP3:
            case APP4:
            case APP5:
            case APP6:
            case APP7:
            case APP8:
            case APP9:
            case APP10:
            case APP11:
            case APP12:
            case APP13:
            case APP14:
            case APP15:
            case COM: {
                JPEGD_VERBOSE_LOG("APPn, currentByte:0x%x", currentByte);
                currentBytes = jpegd_get_two_bytes(pStream);
                if (currentBytes == STRM_ERROR) {
                    JPEGD_ERROR_LOG("Read bits ");
                    return (JPEGDEC_STRM_ERROR);
                }
                /* jump over not supported header */
                if (currentBytes != 0) {
                    pSyntax->stream.readBits += ((currentBytes * 8) - 16);
                    pSyntax->stream.pCurrPos += (((currentBytes * 8) - 16) / 8);
                }
                break;
            }
            default:
                break;
            }
        } else {
            if (currentByte == 0xFFFFFFFF) {
                break;
            }
        }

        if (pSyntax->image.headerReady)
            break;
    } while ((pSyntax->stream.readBits >> 3) <= pSyntax->stream.streamLength);

    if (!findhufftable) {
        JPEGD_VERBOSE_LOG("do not find Huffman Tables");
        jpegd_default_huffman_tables(pCtx);
    }

    return MPP_OK;
    FUN_TEST("Exit");
}

MPP_RET jpegd_allocate_frame(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    if (pCtx->frame_slot_index == -1) {
        RK_U32 value;
        MppFrameFormat fmt = MPP_FMT_YUV420SP;

        switch (pCtx->pSyntax->info.yCbCrMode) {
        case JPEGDEC_YUV420: {
            fmt = MPP_FMT_YUV420SP;
        } break;
        case JPEGDEC_YUV422: {
            fmt = MPP_FMT_YUV422SP;
        } break;
        default : {
            fmt = MPP_FMT_YUV420SP;
        } break;
        }

        mpp_frame_set_fmt(pCtx->output_frame, fmt);
        mpp_frame_set_width(pCtx->output_frame, pCtx->pSyntax->frame.X);
        mpp_frame_set_height(pCtx->output_frame, pCtx->pSyntax->frame.Y);
        mpp_frame_set_hor_stride(pCtx->output_frame, pCtx->pSyntax->frame.hwX);
        mpp_frame_set_ver_stride(pCtx->output_frame, pCtx->pSyntax->frame.hwY);
        mpp_frame_set_pts(pCtx->output_frame, pCtx->pts);

        if (pCtx->eos)
            mpp_frame_set_eos(pCtx->output_frame, 1);

        mpp_buf_slot_get_unused(pCtx->frame_slots, &pCtx->frame_slot_index);
        JPEGD_INFO_LOG("frame_slot_index:%d, X:%d, Y:%d", pCtx->frame_slot_index, pCtx->pSyntax->frame.X, pCtx->pSyntax->frame.Y);

        value = 2;
        mpp_slots_set_prop(pCtx->frame_slots, SLOTS_NUMERATOR, &value);
        value = 1;
        mpp_slots_set_prop(pCtx->frame_slots, SLOTS_DENOMINATOR, &value);
        mpp_buf_slot_set_prop(pCtx->frame_slots, pCtx->frame_slot_index, SLOT_FRAME, pCtx->output_frame);
        mpp_buf_slot_set_flag(pCtx->frame_slots, pCtx->frame_slot_index, SLOT_CODEC_USE);
        mpp_buf_slot_set_flag(pCtx->frame_slots, pCtx->frame_slot_index, SLOT_HAL_OUTPUT);
    }

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_update_frame(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    mpp_buf_slot_clr_flag(pCtx->frame_slots, pCtx->frame_slot_index, SLOT_CODEC_USE);
    pCtx->frame_slot_index = -1;

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_decode_frame(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    JpegSyntaxParam *pSyntax = pCtx->pSyntax;

    /* Store the stream parameters */
    if (pSyntax->info.progressiveScanReady == 0 &&
        pSyntax->info.nonInterleavedScanReady == 0) {
        pSyntax->stream.bitPosInByte = 0;
        pSyntax->stream.pCurrPos = (RK_U8 *) mpp_packet_get_data(pCtx->input_packet);
        pSyntax->stream.pStartOfStream = (RK_U8 *)mpp_packet_get_data(pCtx->input_packet);
        pSyntax->stream.readBits = 0;
        pSyntax->stream.streamLength = (RK_U32)mpp_packet_get_size(pCtx->input_packet);
        pSyntax->stream.appnFlag = 0;
    } else {
        pSyntax->image.headerReady = 0;
    }

    if (pSyntax->info.operationType == JPEGDEC_PROGRESSIVE) {
        JPEGD_ERROR_LOG ("Operation type not supported");
        return (JPEGDEC_UNSUPPORTED);
    }

    /* check if frame size over 16M */
    if ((pSyntax->frame.hwX * pSyntax->frame.hwY) > JPEGDEC_MAX_PIXEL_AMOUNT) {
        JPEGD_ERROR_LOG ("Resolution > 16M ==> use slice mode!");
        return (JPEGDEC_PARAM_ERROR);
    }

    /* check if input streaming used */
    if (!pSyntax->info.SliceReadyForPause &&
        !pSyntax->info.inputBufferEmpty && pCtx->bufferSize) {
        pSyntax->info.inputStreaming = 1;
        pSyntax->info.inputBufferLen = pCtx->streamLength;
        pSyntax->info.decodedStreamLen += pSyntax->info.inputBufferLen;
    }

    ret = jpegd_decode_frame_impl(pCtx);

    FUN_TEST("Exit");
    return ret;
}

void jpegd_free_huffman_tables(void *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;

    if (JpegParserCtx->pSyntax->vlc.acTable0.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.acTable0.vals);
        JpegParserCtx->pSyntax->vlc.acTable0.vals = NULL;
    }

    if (JpegParserCtx->pSyntax->vlc.acTable1.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.acTable1.vals);
        JpegParserCtx->pSyntax->vlc.acTable1.vals = NULL;
    }

    if (JpegParserCtx->pSyntax->vlc.acTable2.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.acTable2.vals);
        JpegParserCtx->pSyntax->vlc.acTable2.vals = NULL;
    }

    if (JpegParserCtx->pSyntax->vlc.acTable3.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.acTable3.vals);
        JpegParserCtx->pSyntax->vlc.acTable3.vals = NULL;
    }

    if (JpegParserCtx->pSyntax->vlc.dcTable0.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.dcTable0.vals);
        JpegParserCtx->pSyntax->vlc.dcTable0.vals = NULL;
    }

    if (JpegParserCtx->pSyntax->vlc.dcTable1.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.dcTable1.vals);
        JpegParserCtx->pSyntax->vlc.dcTable1.vals = NULL;
    }

    if (JpegParserCtx->pSyntax->vlc.dcTable2.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.dcTable2.vals);
        JpegParserCtx->pSyntax->vlc.dcTable2.vals = NULL;
    }

    if (JpegParserCtx->pSyntax->vlc.dcTable3.vals) {
        mpp_free(JpegParserCtx->pSyntax->vlc.dcTable3.vals);
        JpegParserCtx->pSyntax->vlc.dcTable3.vals = NULL;
    }

    FUN_TEST("Exit");
}


MPP_RET jpegd_parse(void *ctx, HalDecTask *task)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;
    task->valid = 0;

    jpegd_free_huffman_tables(JpegParserCtx);
    memset(JpegParserCtx->pSyntax, 0, sizeof(JpegSyntaxParam));
    jpegd_get_image_info(JpegParserCtx);
    ret = jpegd_decode_frame(JpegParserCtx);

    if (MPP_OK == ret) {
        jpegd_allocate_frame(JpegParserCtx);
        task->syntax.data = (void *)JpegParserCtx->pSyntax;
        task->syntax.number = sizeof(JpegSyntaxParam);
        task->output = JpegParserCtx->frame_slot_index;
        task->valid = 1;
        jpegd_update_frame(JpegParserCtx);
    }

    //mpp_show_mem_status();

    FUN_TEST("Exit");
    return ret;
}

MPP_RET jpegd_deinit(void *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;

    if (JpegParserCtx->recv_buffer) {
        mpp_free(JpegParserCtx->recv_buffer);
        JpegParserCtx->recv_buffer = NULL;
    }

    jpegd_free_huffman_tables(JpegParserCtx);

    if (JpegParserCtx->pSyntax) {
        mpp_free(JpegParserCtx->pSyntax);
        JpegParserCtx->pSyntax = NULL;
    }

    if (JpegParserCtx->output_frame) {
        mpp_free(JpegParserCtx->output_frame);
    }

    if (JpegParserCtx->input_packet) {
        mpp_packet_deinit(&JpegParserCtx->input_packet);
    }

    JpegParserCtx->output_fmt = MPP_FMT_YUV420SP;
    JpegParserCtx->pts = 0;
    JpegParserCtx->eos = 0;
    JpegParserCtx->parser_debug_enable = 0;
    JpegParserCtx->input_jpeg_count = 0;

    FUN_TEST("Exit");
    return 0;
}

MPP_RET reset_jpeg_parser_context(JpegParserContext *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *pCtx = ctx;
    if (NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    pCtx->packet_slots = NULL;
    pCtx->frame_slots = NULL;
    pCtx->recv_buffer = NULL;

    /* resolution */
    pCtx->minSupportedWidth = 0;
    pCtx->minSupportedHeight = 0;
    pCtx->maxSupportedWidth = 0;
    pCtx->maxSupportedHeight = 0;
    pCtx->maxSupportedPixelAmount = 0;
    pCtx->maxSupportedSliceSize = 0;

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_init(void *ctx, ParserCfg *parser_cfg)
{
    FUN_TEST("Enter");
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;
    if (NULL == JpegParserCtx) {
        JpegParserCtx = (JpegParserContext *)mpp_calloc(JpegParserContext, 1);
        if (NULL == JpegParserCtx) {
            JPEGD_ERROR_LOG("NULL pointer");
            return MPP_ERR_NULL_PTR;
        }
    }
    mpp_env_get_u32("jpegd_log", &jpegd_log, JPEGD_ERR_LOG);

    reset_jpeg_parser_context(JpegParserCtx);
    JpegParserCtx->frame_slots = parser_cfg->frame_slots;
    JpegParserCtx->packet_slots = parser_cfg->packet_slots;
    JpegParserCtx->frame_slot_index = -1;
    mpp_buf_slot_setup(JpegParserCtx->frame_slots, 1);

    JpegParserCtx->recv_buffer = mpp_calloc(RK_U8, JPEGD_STREAM_BUFF_SIZE);
    if (NULL == JpegParserCtx->recv_buffer) {
        JPEGD_ERROR_LOG("no memory!");
        return MPP_ERR_NOMEM;
    }
    JpegParserCtx->bufferSize = JPEGD_STREAM_BUFF_SIZE;
    mpp_packet_init(&JpegParserCtx->input_packet, JpegParserCtx->recv_buffer, JPEGD_STREAM_BUFF_SIZE);

    mpp_frame_init(&JpegParserCtx->output_frame);
    if (!JpegParserCtx->output_frame) {
        JPEGD_ERROR_LOG("Failed to allocate output frame buffer");
        return MPP_ERR_NOMEM;
    }

    JpegParserCtx->pSyntax = mpp_calloc(JpegSyntaxParam, 1);
    if (NULL == JpegParserCtx->pSyntax) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    memset(JpegParserCtx->pSyntax, 0, sizeof(JpegSyntaxParam));
    JpegParserCtx->pSyntax->ppInstance = (void *)0; /* will be changed when need pp */

    JpegParserCtx->decImageType = JPEGDEC_IMAGE; /* FULL MODEs */
    JpegParserCtx->output_fmt = MPP_FMT_YUV420SP;

    /* max */
    JpegParserCtx->maxSupportedWidth = JPEGDEC_MAX_WIDTH_8190;
    JpegParserCtx->maxSupportedHeight = JPEGDEC_MAX_HEIGHT_8190;
    JpegParserCtx->maxSupportedPixelAmount = JPEGDEC_MAX_PIXEL_AMOUNT_8190;
    JpegParserCtx->maxSupportedSliceSize = JPEGDEC_MAX_SLICE_SIZE_8190;

    /* min */
    JpegParserCtx->minSupportedWidth = JPEGDEC_MIN_WIDTH;
    JpegParserCtx->minSupportedHeight = JPEGDEC_MIN_HEIGHT;

    JpegParserCtx->extensionsSupported = 1;

    JpegParserCtx->pts = 0;
    JpegParserCtx->eos = 0;
    JpegParserCtx->parser_debug_enable = 0;
    JpegParserCtx->input_jpeg_count = 0;

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_flush(void *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;
    (void)JpegParserCtx;
    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_reset(void *ctx)
{
    FUN_TEST("Enter");
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;

    (void)JpegParserCtx;

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_control(void *ctx, RK_S32 cmd, void *param)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegParserContext *JpegParserCtx = (JpegParserContext *)ctx;
    if (NULL == JpegParserCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd) {
    case MPP_DEC_SET_OUTPUT_FORMAT: {
        JpegParserCtx->output_fmt = *((RK_U32 *)param);
        JPEGD_INFO_LOG("output_format:%d\n", JpegParserCtx->output_fmt);
    } break;
    default :
        ret = MPP_NOK;
    }
    FUN_TEST("Exit");
    return ret;
}

MPP_RET jpegd_callback(void *ctx, void *err_info)
{
    FUN_TEST("Enter");
    (void) ctx;
    (void) err_info;
    FUN_TEST("Exit");
    return MPP_OK;
}

const ParserApi api_jpegd_parser = {
    "jpegd_parse",
    MPP_VIDEO_CodingMJPEG,
    sizeof(JpegParserContext),
    0,
    jpegd_init,
    jpegd_deinit,
    jpegd_prepare,
    jpegd_parse,
    jpegd_reset,
    jpegd_flush,
    jpegd_control,
    jpegd_callback,
};


