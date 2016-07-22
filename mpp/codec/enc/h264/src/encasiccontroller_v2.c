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
    Include headers
------------------------------------------------------------------------------*/
#include "encpreprocess.h"
#include "encasiccontroller.h"
#include "enccommon.h"
#include "ewl.h"
#include "mpp_log.h"
/*------------------------------------------------------------------------------
    External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

/* Mask fields */
#define mask_2b         (u32)0x00000003
#define mask_3b         (u32)0x00000007
#define mask_4b         (u32)0x0000000F
#define mask_5b         (u32)0x0000001F
#define mask_6b         (u32)0x0000003F
#define mask_11b        (u32)0x000007FF
#define mask_14b        (u32)0x00003FFF
#define mask_16b        (u32)0x0000FFFF

#define HSWREG(n)       ((n)*4)

/*------------------------------------------------------------------------------
    Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    EncAsicMemAlloc_V2

    Allocate HW/SW shared memory

    Input:
        asic        asicData structure
        width       width of encoded image, multiple of four
        height      height of encoded image
        type        ASIC_MPEG4 / ASIC_H263 / ASIC_JPEG

    Output:
        asic        base addresses point to allocated memory areas

    Return:
        ENCHW_OK        Success.
        ENCHW_NOK       Error: memory allocation failed, no memories allocated
                        and EWL instance released

------------------------------------------------------------------------------*/
i32 EncAsicMemAlloc_V2(asicData_s * asic, u32 width, u32 height,
                       u32 encodingType)
{
    u32 mbTotal;
    regValues_s *regs;
    MppBuffer *buff = NULL;

    ASSERT(asic != NULL);
    ASSERT(width != 0);
    ASSERT(height != 0);
    ASSERT((height % 2) == 0);
    ASSERT((width % 4) == 0);

    regs = &asic->regs;

    regs->codingType = encodingType;

    width = (width + 15) / 16;
    height = (height + 15) / 16;

    mbTotal = width * height;

    if (regs->codingType != ASIC_JPEG) {
        /* The sizes of the memories */
        u32 internalImageLumaSize = mbTotal * (16 * 16);

        u32 internalImageChromaSize = mbTotal * (2 * 8 * 8);

        if (asic->asicDataBufferGroup == NULL) {
            if (mpp_buffer_group_get_internal((&(asic->asicDataBufferGroup)), MPP_BUFFER_TYPE_ION) != MPP_OK) {
                mpp_err("mpeg4d mpp_buffer_group_get failed\n");
                return ENCHW_NOK;
            }
        }

        if (asic->internalImageLuma[0] == NULL) {
            if (mpp_buffer_get(asic->asicDataBufferGroup, &(asic->internalImageLuma[0]), (internalImageLumaSize + 4096)) != MPP_OK) {
                mpp_err("asic->internalImageLuma[0] alloc failed!\n");
                return ENCHW_NOK;
            }
        }

        if (asic->internalImageChroma[0] == NULL) {
            if (mpp_buffer_get(asic->asicDataBufferGroup, &(asic->internalImageChroma[0]), (internalImageChromaSize + 4096)) != MPP_OK) {
                mpp_err("asic->internalImageChroma[0] alloc failed!\n");
                return ENCHW_NOK;
            }
        }

        if (asic->internalImageLuma[1] == NULL) {
            if (mpp_buffer_get(asic->asicDataBufferGroup, &(asic->internalImageLuma[1]), (internalImageLumaSize + 4096)) != MPP_OK) {
                mpp_err("asic->internalImageLuma[1] alloc failed!\n");
                return ENCHW_NOK;
            }
        }

        if (asic->internalImageChroma[1] == NULL) {
            if (mpp_buffer_get(asic->asicDataBufferGroup, &(asic->internalImageChroma[1]), (internalImageChromaSize + 4096)) != MPP_OK) {
                mpp_err("asic->internalImageChroma[1] alloc failed!\n");
                return ENCHW_NOK;
            }
        }

        regs->internalImageLumBaseW = mpp_buffer_get_fd(asic->internalImageLuma[0]);
        regs->internalImageChrBaseW = mpp_buffer_get_fd(asic->internalImageChroma[0]);
        regs->internalImageLumBaseR = mpp_buffer_get_fd(asic->internalImageLuma[1]);
        regs->internalImageChrBaseR = mpp_buffer_get_fd(asic->internalImageChroma[1]);

        /* NAL size table, table size must be 64-bit multiple,
         * space for zero at the end of table */
        if (regs->codingType == ASIC_H264) {
            /* Atleast 1 macroblock row in every slice */
            buff = &asic->sizeTbl.nal;
            asic->sizeTblSize = (sizeof(u32) * (height + 1) + 7) & (~7);
        }

        if (mpp_buffer_get(asic->asicDataBufferGroup, buff, asic->sizeTblSize) != MPP_OK) {
            mpp_err("asic->sizeTbl alloc failed!\n");
            return ENCHW_NOK;
        }

        if (regs->riceEnable) {
            u32 bytes = ((width + 11) / 12 * (height * 2 - 1)) * 8;
            if (mpp_buffer_get(asic->asicDataBufferGroup, &(asic->riceRead), bytes) != MPP_OK) {
                mpp_err("asic->riceRead alloc failed!\n");
                return ENCHW_NOK;
            }
            if (mpp_buffer_get(asic->asicDataBufferGroup, &(asic->riceWrite), bytes) != MPP_OK) {
                mpp_err("asic->riceWrite alloc failed!\n");
                return ENCHW_NOK;
            }
            regs->riceReadBase = mpp_buffer_get_fd(asic->riceRead);
            regs->riceWriteBase = mpp_buffer_get_fd(asic->riceWrite);
        }
    }

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    EncAsicMemFree_V2

    Free HW/SW shared memory

------------------------------------------------------------------------------*/
void EncAsicMemFree_V2(asicData_s * asic)
{
    ASSERT(asic != NULL);

    // mask by lance 2016.05.05
    /*if (asic->internalImageLuma[0] != NULL)
        VPUFreeLinear(&asic->internalImageLuma[0]);

    if (asic->internalImageChroma[0] != NULL)
        VPUFreeLinear(&asic->internalImageChroma[0]);

    if (asic->internalImageLuma[1] != NULL)
        VPUFreeLinear(&asic->internalImageLuma[1]);

    if (asic->internalImageChroma[1] != NULL)
        VPUFreeLinear(&asic->internalImageChroma[1]);

    if (asic->sizeTbl.nal.vir_addr != NULL)
        VPUFreeLinear(&asic->sizeTbl.nal);

    if (asic->cabacCtx.vir_addr != NULL)
        VPUFreeLinear(&asic->cabacCtx);

    if (asic->riceRead.vir_addr != NULL)
        VPUFreeLinear(&asic->riceRead);

    if (asic->riceWrite.vir_addr != NULL)
        VPUFreeLinear(&asic->riceWrite);

    asic->internalImageLuma[0] = NULL;
    asic->internalImageChroma[0] = NULL;
    asic->internalImageLuma[1] = NULL;
    asic->internalImageChroma[1] = NULL;
    asic->sizeTbl.nal = NULL;
    asic->cabacCtx = NULL;
    asic->riceRead = NULL;
    asic->riceWrite = NULL;*/
    // --------------
    // add by lance 2016.05.05
    if (asic->internalImageLuma[0] != NULL) {
        mpp_buffer_put(asic->internalImageLuma[0]);
        asic->internalImageLuma[0] = NULL;
    }

    if (asic->internalImageLuma[1] != NULL) {
        mpp_buffer_put(asic->internalImageLuma[1]);
        asic->internalImageLuma[1] = NULL;
    }

    if (asic->internalImageChroma[0] != NULL) {
        mpp_buffer_put(asic->internalImageChroma[0]);
        asic->internalImageChroma[0] = NULL;
    }

    if (asic->internalImageChroma[1] != NULL) {
        mpp_buffer_put(asic->internalImageChroma[1]);
        asic->internalImageChroma[1] = NULL;
    }

    if (asic->sizeTbl.nal != NULL) {
        mpp_buffer_put(asic->sizeTbl.nal);
        asic->sizeTbl.nal = NULL;
    }

    if (asic->riceRead != NULL) {
        mpp_buffer_put(asic->riceRead);
        asic->riceRead = NULL;
    }

    if (asic->riceWrite != NULL) {
        mpp_buffer_put(asic->riceWrite);
        asic->riceWrite = NULL;
    }

    if (asic->asicDataBufferGroup != NULL) {
        mpp_log("asic->asicDataBufferGroup free!\n");
        mpp_buffer_group_put(asic->asicDataBufferGroup);
        asic->asicDataBufferGroup = NULL;
    }
    // --------------
}

