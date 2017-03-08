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
#define MODULE_TAG "HAL_JPEGD_COMMON"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "mpp_buffer.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_dec.h"
#include "mpp_buffer.h"
#include "mpp_env.h"
#include "mpp_bitput.h"

#include "jpegd_syntax.h"
#include "jpegd_api.h"
#include "hal_jpegd_common.h"

MPP_RET jpegd_allocate_chroma_out_buffer(JpegSyntaxParam *pSyntax)
{
    FUN_TEST("Enter");
    if (NULL == pSyntax) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    if (pSyntax->ppInstance == NULL || (pSyntax->ppInstance != NULL
                                        && !pSyntax->ppControl.usePipeline)) {
        if (pSyntax->info.givenOutLuma.vir_addr == NULL) {
            JPEGD_INFO_LOG("givenOutLuma is NULL");

            /* luma bus address to output */
            pSyntax->info.outLuma = pSyntax->asicBuff.outLumaBuffer;

            if (pSyntax->image.sizeChroma) {
                JPEGD_INFO_LOG("sizeChroma:%d", pSyntax->image.sizeChroma);
                JPEGD_INFO_LOG("sizeLuma:%d, outLumaBuffer.phy_addr:%x",
                               pSyntax->image.sizeLuma,
                               pSyntax->asicBuff.outLumaBuffer.phy_addr);

                pSyntax->asicBuff.outChromaBuffer.phy_addr =
                    pSyntax->asicBuff.outLumaBuffer.phy_addr
                    + (pSyntax->image.sizeLuma << 10);

                if (pSyntax->info.operationType != JPEGDEC_BASELINE) {
                    pSyntax->asicBuff.outChromaBuffer2.phy_addr =
                        pSyntax->asicBuff.outChromaBuffer.phy_addr
                        + ((pSyntax->image.sizeChroma / 2) << 10);
                } else {
                    pSyntax->asicBuff.outChromaBuffer2.phy_addr = 0;
                }

                // chroma bus address to output
                pSyntax->info.outChroma = pSyntax->asicBuff.outChromaBuffer;
                pSyntax->info.outChroma2 = pSyntax->asicBuff.outChromaBuffer2;
            }
        } else {
            JPEGD_INFO_LOG("givenOutLuma is not NULL");
            pSyntax->asicBuff.outLumaBuffer.vir_addr =
                pSyntax->info.givenOutLuma.vir_addr;
            pSyntax->asicBuff.outLumaBuffer.phy_addr =
                pSyntax->info.givenOutLuma.phy_addr;

            pSyntax->asicBuff.outChromaBuffer.vir_addr =
                pSyntax->info.givenOutChroma.vir_addr;
            pSyntax->asicBuff.outChromaBuffer.phy_addr =
                pSyntax->info.givenOutChroma.phy_addr;
            pSyntax->asicBuff.outChromaBuffer2.vir_addr =
                pSyntax->info.givenOutChroma2.vir_addr;
            pSyntax->asicBuff.outChromaBuffer2.phy_addr =
                pSyntax->info.givenOutChroma2.phy_addr;

            /* luma bus address to output */
            pSyntax->info.outLuma = pSyntax->asicBuff.outLumaBuffer;

            if (pSyntax->image.sizeChroma) {
                // chroma bus address to output
                pSyntax->info.outChroma = pSyntax->asicBuff.outChromaBuffer;
                pSyntax->info.outChroma2 = pSyntax->asicBuff.outChromaBuffer2;
            }

            /* flag to release */
            pSyntax->info.userAllocMem = 1;
        }

        JPEGD_INFO_LOG("Luma virtual: %p, phy_addr: %x\n",
                       pSyntax->asicBuff.outLumaBuffer.vir_addr,
                       pSyntax->asicBuff.outLumaBuffer.phy_addr);
        JPEGD_INFO_LOG("Chroma virtual: %p, phy_addr: %x\n",
                       pSyntax->asicBuff.outChromaBuffer.vir_addr,
                       pSyntax->asicBuff.outChromaBuffer.phy_addr);
    }
    FUN_TEST("Exit");
    return MPP_OK;
}

void jpegd_write_tables(JpegSyntaxParam *pSyntax, JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    ScanInfo         *JPG_SCN = &pSyntax->scan;
    HuffmanTables    *JPG_VLC = &pSyntax->vlc;
    QuantTables      *JPG_QTB = &pSyntax->quant;
    FrameInfo        *JPG_FRM = &pSyntax->frame;

    RK_U32 i, j = 0;
    RK_U32 shifter = 32;
    RK_U32 tableWord = 0;
    RK_U32 tableValue = 0;
    RK_U8 tableTmp[64] = { 0 };
    RK_U32 *pTableBase = NULL;

    pTableBase = (RK_U32 *)mpp_buffer_get_ptr(pCtx->pTableBase);

    /* QP tables for all components */
    for (j = 0; j < pSyntax->info.amountOfQTables; j++) {
        if ((JPG_FRM->component[j].Tq) == 0) {
            for (i = 0; i < 64; i++) {
                tableTmp[zzOrder[i]] = (RK_U8) JPG_QTB->table0[i];
            }

            /* update shifter */
            shifter = 32;

            for (i = 0; i < 64; i++) {
                shifter -= 8;

                if (shifter == 24)
                    tableWord = (tableTmp[i] << shifter);
                else
                    tableWord |= (tableTmp[i] << shifter);

                if (shifter == 0) {
                    *(pTableBase) = tableWord;
                    pTableBase++;
                    shifter = 32;
                }
            }
        } else {
            for (i = 0; i < 64; i++) {
                tableTmp[zzOrder[i]] = (RK_U8) JPG_QTB->table1[i];
            }

            /* update shifter */
            shifter = 32;

            for (i = 0; i < 64; i++) {
                shifter -= 8;

                if (shifter == 24)
                    tableWord = (tableTmp[i] << shifter);
                else
                    tableWord |= (tableTmp[i] << shifter);

                if (shifter == 0) {
                    *(pTableBase) = tableWord;
                    pTableBase++;
                    shifter = 32;
                }
            }
        }
    }

    /* update shifter */
    shifter = 32;

    if (pSyntax->info.yCbCrMode != JPEGDEC_YUV400) {
        /* this trick is done because hw always wants luma table as ac hw table 1 */
        if (JPG_SCN->Ta[0] == 0) {
            /* Write AC Table 1 (as specified in HW regs)
             * NOTE: Not the same as actable[1] (as specified in JPEG Spec) */
            JPEGD_VERBOSE_LOG("INTERNAL: Write tables: AC1 (luma)\n");
            if (JPG_VLC->acTable0.vals) {
                for (i = 0; i < 162; i++) {
                    if (i < JPG_VLC->acTable0.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->acTable0.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 162; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }

            /* Write AC Table 2 */
            JPEGD_VERBOSE_LOG("INTERNAL: Write tables: AC2 (not-luma)\n");
            if (JPG_VLC->acTable1.vals) {
                for (i = 0; i < 162; i++) {
                    if (i < JPG_VLC->acTable1.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->acTable1.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 162; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }
        } else {
            /* Write AC Table 1 (as specified in HW regs)
             * NOTE: Not the same as actable[1] (as specified in JPEG Spec) */
            if (JPG_VLC->acTable1.vals) {
                JPEGD_INFO_LOG("INTERNAL: Write tables: AC1 (luma)\n");
                for (i = 0; i < 162; i++) {
                    if (i < JPG_VLC->acTable1.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->acTable1.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 162; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }

            /* Write AC Table 2 */
            if (JPG_VLC->acTable0.vals) {
                JPEGD_INFO_LOG("INTERNAL: writeTables: AC2 (not-luma)\n");
                for (i = 0; i < 162; i++) {
                    if (i < JPG_VLC->acTable0.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->acTable0.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 162; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }
        }

        /* this trick is done because hw always wants luma table as dc hw table 1 */
        if (JPG_SCN->Td[0] == 0) {
            if (JPG_VLC->dcTable0.vals) {
                for (i = 0; i < 12; i++) {
                    if (i < JPG_VLC->dcTable0.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->dcTable0.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 12; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }

            if (JPG_VLC->dcTable1.vals) {
                for (i = 0; i < 12; i++) {
                    if (i < JPG_VLC->dcTable1.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->dcTable1.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 12; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }

        } else {
            if (JPG_VLC->dcTable1.vals) {
                for (i = 0; i < 12; i++) {
                    if (i < JPG_VLC->dcTable1.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->dcTable1.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 12; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }

            if (JPG_VLC->dcTable0.vals) {
                for (i = 0; i < 12; i++) {
                    if (i < JPG_VLC->dcTable0.tableLength) {
                        tableValue = (RK_U8) JPG_VLC->dcTable0.vals[i];
                    } else {
                        tableValue = 0;
                    }

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                for (i = 0; i < 12; i++) {
                    tableWord = 0;
                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }
        }
    } else { /* YUV400 */
        if (!pSyntax->info.nonInterleavedScanReady) {
            /*this trick is done because hw always wants luma table as ac hw table 1 */
            if (JPG_SCN->Ta[0] == 0) {
                /* Write AC Table 1 (as specified in HW regs)
                 * NOTE: Not the same as actable[1] (as specified in JPEG Spec) */
                JPEGD_INFO_LOG("INTERNAL: Write tables: AC1 (luma)\n");
                if (JPG_VLC->acTable0.vals) {
                    for (i = 0; i < 162; i++) {
                        if (i < JPG_VLC->acTable0.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->acTable0.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 162; i++) {
                        tableWord = 0;
                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                /* Write AC Table 2 */
                JPEGD_INFO_LOG("INTERNAL: Write zero table (YUV400): \n");
                for (i = 0; i < 162; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                /* Write AC Table 1 (as specified in HW regs)
                 * NOTE: Not the same as actable[1] (as specified in JPEG Spec) */
                if (JPG_VLC->acTable1.vals) {
                    JPEGD_INFO_LOG("INTERNAL: Write tables: AC1 (luma)\n");
                    for (i = 0; i < 162; i++) {
                        if (i < JPG_VLC->acTable1.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->acTable1.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 162; i++) {
                        tableWord = 0;

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                /* Write AC Table 2 */
                JPEGD_INFO_LOG("INTERNAL: writeTables: padding zero (YUV400)\n");
                for (i = 0; i < 162; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }

            /* this trick is done because hw always wants luma table as dc hw table 1 */
            if (JPG_SCN->Td[0] == 0) {
                if (JPG_VLC->dcTable0.vals) {
                    for (i = 0; i < 12; i++) {
                        if (i < JPG_VLC->dcTable0.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->dcTable0.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 12; i++) {
                        tableWord = 0;
                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                for (i = 0; i < 12; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                if (JPG_VLC->dcTable1.vals) {
                    for (i = 0; i < 12; i++) {
                        if (i < JPG_VLC->dcTable1.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->dcTable1.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 12; i++) {
                        tableWord = 0;
                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                for (i = 0; i < 12; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }
        } else {
            /* this trick is done because hw always wants luma table as ac hw table 1 */
            if (JPG_SCN->Ta[pSyntax->info.componentId] == 0) {
                /* Write AC Table 1 (as specified in HW regs)
                 * NOTE: Not the same as actable[1] (as specified in JPEG Spec) */
                JPEGD_INFO_LOG("INTERNAL: Write tables: AC1 (luma)\n");
                if (JPG_VLC->acTable0.vals) {
                    for (i = 0; i < 162; i++) {
                        if (i < JPG_VLC->acTable0.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->acTable0.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 162; i++) {
                        tableWord = 0;
                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                /* Write AC Table 2 */
                JPEGD_INFO_LOG("INTERNAL: Write zero table (YUV400): \n");
                for (i = 0; i < 162; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                /* Write AC Table 1 (as specified in HW regs)
                 * NOTE: Not the same as actable[1] (as specified in JPEG Spec) */
                if (JPG_VLC->acTable1.vals) {
                    JPEGD_INFO_LOG("INTERNAL: Write tables: AC1 (luma)\n");
                    for (i = 0; i < 162; i++) {
                        if (i < JPG_VLC->acTable1.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->acTable1.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 162; i++) {
                        tableWord = 0;

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                /* Write AC Table 2 */
                JPEGD_INFO_LOG("INTERNAL: writeTables: padding zero (YUV400)\n");
                for (i = 0; i < 162; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }

            /* this trick is done because hw always wants luma table as dc hw table 1 */
            if (JPG_SCN->Td[pSyntax->info.componentId] == 0) {
                if (JPG_VLC->dcTable0.vals) {
                    for (i = 0; i < 12; i++) {
                        if (i < JPG_VLC->dcTable0.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->dcTable0.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 12; i++) {
                        tableWord = 0;
                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                for (i = 0; i < 12; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            } else {
                if (JPG_VLC->dcTable1.vals) {
                    for (i = 0; i < 12; i++) {
                        if (i < JPG_VLC->dcTable1.tableLength) {
                            tableValue = (RK_U8) JPG_VLC->dcTable1.vals[i];
                        } else {
                            tableValue = 0;
                        }

                        if (shifter == 32)
                            tableWord = (tableValue << (shifter - 8));
                        else
                            tableWord |= (tableValue << (shifter - 8));

                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                } else {
                    for (i = 0; i < 12; i++) {
                        tableWord = 0;
                        shifter -= 8;

                        if (shifter == 0) {
                            *(pTableBase) = tableWord;
                            pTableBase++;
                            shifter = 32;
                        }
                    }
                }

                for (i = 0; i < 12; i++) {
                    tableValue = 0;

                    if (shifter == 32)
                        tableWord = (tableValue << (shifter - 8));
                    else
                        tableWord |= (tableValue << (shifter - 8));

                    shifter -= 8;

                    if (shifter == 0) {
                        *(pTableBase) = tableWord;
                        pTableBase++;
                        shifter = 32;
                    }
                }
            }
        }

    }

    for (i = 0; i < 4; i++) {
        tableValue = 0;

        if (shifter == 32)
            tableWord = (tableValue << (shifter - 8));
        else
            tableWord |= (tableValue << (shifter - 8));

        shifter -= 8;

        if (shifter == 0) {
            *(pTableBase) = tableWord;
            pTableBase++;
            shifter = 32;
        }
    }
    FUN_TEST("Exit");
}

MPP_RET jpegd_set_output_format(JpegHalContext *pCtx, JpegSyntaxParam *pSyntax)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    if (NULL == pSyntax || NULL == pCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }
    PostProcessInfo ppInfo;
    RK_U32 ppInputFomart = 0;
    RK_U32 ppScaleW = 640, ppScaleH = 480;

    if (pCtx->set_output_fmt_flag
        && (pCtx->output_fmt != pSyntax->imageInfo.outputFormat)) {
        /* Using pp to convert all format to yuv420sp */
        switch (pSyntax->imageInfo.outputFormat) {
        case JPEGDEC_YCbCr400:
            ppInputFomart = PP_IN_FORMAT_YUV400;
            break;
        case MPP_FMT_YUV420SP:
            ppInputFomart = PP_IN_FORMAT_YUV420SEMI;
            break;
        case MPP_FMT_YUV422SP:
            ppInputFomart = PP_IN_FORMAT_YUV422SEMI;
            break;
        case JPEGDEC_YCbCr440:
            ppInputFomart = PP_IN_FORMAT_YUV440SEMI;
            break;
        case JPEGDEC_YCbCr411_SEMIPLANAR:
            ppInputFomart = PP_IN_FORMAT_YUV411_SEMI;
            break;
        case JPEGDEC_YCbCr444_SEMIPLANAR:
            ppInputFomart = PP_IN_FORMAT_YUV444_SEMI;
            break;
        }

        // set pp info
        memset(&ppInfo, 0, sizeof(ppInfo));
        ppInfo.enable = 1;
        ppInfo.outFomart = 5;   //PP_OUT_FORMAT_YUV420INTERLAVE
        ppScaleW = pSyntax->imageInfo.outputWidth;
        ppScaleH = pSyntax->imageInfo.outputHeight;
        if (ppScaleW > 1920) { // || ppScaleH > 1920) {
            ppScaleW = (ppScaleW + 15) & (~15); //(ppScaleW + 15)/16*16;
            ppScaleH = (ppScaleH + 15) & (~15);
        } else {
            /* pp dest width must be dividable by 8 */
            ppScaleW = (ppScaleW + 7) & (~7);
            /*
            * must be dividable by 2.in pp downscaling,
            * the output lines always equal (desire lines - 1);
            */
            ppScaleH = (ppScaleH + 1) & (~1);
        }

        JPEGD_INFO_LOG("Post Process! ppScaleW:%d, ppScaleH:%d",
                       ppScaleW, ppScaleH);
        pSyntax->ppInstance = (void *)1;
    } else {
        /* keep original output format */
        memset(&ppInfo, 0, sizeof(ppInfo));
        ppInfo.outFomart = 5;   //PP_OUT_FORMAT_YUV420INTERLAVE
        ppScaleW = pSyntax->imageInfo.outputWidth;
        ppScaleH = pSyntax->imageInfo.outputHeight;

        ppScaleW = (ppScaleW + 15) & (~15);
        ppScaleH = (ppScaleH + 15) & (~15);

        switch (pSyntax->imageInfo.outputFormat) {
        case JPEGDEC_YCbCr400:
            ppInputFomart = PP_IN_FORMAT_YUV400;
            break;
        case MPP_FMT_YUV420SP:
            ppInputFomart = PP_IN_FORMAT_YUV420SEMI;
            break;
        case MPP_FMT_YUV422SP:
            ppInputFomart = PP_IN_FORMAT_YUV422SEMI;
            break;
        case JPEGDEC_YCbCr440:
            ppInputFomart = PP_IN_FORMAT_YUV440SEMI;
            break;
        case JPEGDEC_YCbCr411_SEMIPLANAR:
            ppInputFomart = PP_IN_FORMAT_YUV411_SEMI;
            break;
        case JPEGDEC_YCbCr444_SEMIPLANAR:
            ppInputFomart = PP_IN_FORMAT_YUV444_SEMI;
            break;
        }

        pSyntax->ppInstance = (void *)0;
    }

    memcpy(&(pSyntax->ppInfo), &(ppInfo), sizeof(PostProcessInfo));
    pSyntax->ppScaleW = ppScaleW;
    pSyntax->ppScaleH = ppScaleH;
    pSyntax->ppInputFomart = ppInputFomart;

    FUN_TEST("Exit");
    return  ret;
}
