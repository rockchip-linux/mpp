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

#ifndef __EWL_H__
#define __EWL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
//#include "vpu_mem.h"  // mask by lance 2016.05.05
#include "vpu.h"
//#include "vpu_drv.h"

/* Return values */
#define EWL_OK                      0
#define EWL_ERROR                  -1

#define EWL_HW_WAIT_OK              EWL_OK
#define EWL_HW_WAIT_ERROR           EWL_ERROR
#define EWL_HW_WAIT_TIMEOUT         1

/* HW configuration values */
#define EWL_HW_BUS_TYPE_UNKNOWN     0
#define EWL_HW_BUS_TYPE_AHB         1
#define EWL_HW_BUS_TYPE_OCP         2
#define EWL_HW_BUS_TYPE_AXI         3
#define EWL_HW_BUS_TYPE_PCI         4

#define EWL_HW_BUS_WIDTH_UNKNOWN     0
#define EWL_HW_BUS_WIDTH_32BITS      1
#define EWL_HW_BUS_WIDTH_64BITS      2
#define EWL_HW_BUS_WIDTH_128BITS     3

#define EWL_HW_SYNTHESIS_LANGUAGE_UNKNOWN     0
#define EWL_HW_SYNTHESIS_LANGUAGE_VHDL        1
#define EWL_HW_SYNTHESIS_LANGUAGE_VERIALOG     2

#define EWL_HW_CONFIG_NOT_SUPPORTED    0
#define EWL_HW_CONFIG_ENABLED          1

/* Hardware configuration description */
typedef struct EWLHwConfig {
    u32 maxEncodedWidth; /* Maximum supported width for video encoding (not JPEG) */
    u32 h264Enabled;     /* HW supports H.264 */
    u32 jpegEnabled;     /* HW supports JPEG */
    u32 mpeg4Enabled;    /* HW supports MPEG-4 */
    u32 vsEnabled;       /* HW supports video stabilization */
    u32 rgbEnabled;      /* HW supports RGB input */
    u32 busType;         /* HW bus type in use */
    u32 busWidth;
    u32 synthesisLanguage;
} EWLHwConfig_t;

/* Allocated linear memory area information */
/*    typedef struct EWLLinearMem
    {
        u32 *vir_addr;
        u32 phy_addr;
        u32 size;
    } VPULinearMem_t;*/

/* EWLInitParam is used to pass parameters when initializing the EWL */
typedef struct EWLInitParam {
    u32 clientType;
} EWLInitParam_t;

#define EWL_CLIENT_TYPE_H264_ENC         1U
#define EWL_CLIENT_TYPE_MPEG4_ENC        2U
#define EWL_CLIENT_TYPE_JPEG_ENC         3U
#define EWL_CLIENT_TYPE_VIDEOSTAB        4U

/*------------------------------------------------------------------------------
    4.  Function prototypes
------------------------------------------------------------------------------*/

/* Read and return the HW ID register value, static implementation */
u32 EWLReadAsicID(void);

/* Read and return HW configuration info, static implementation */
//    EWLHwConfig_t EWLReadAsicConfig(void);

/* Initialize the EWL instance
 * Returns a wrapper instance or NULL for error
 * EWLInit is called when the encoder instance is initialized */
//    const void *EWLInit(EWLInitParam_t * param);

/* Release the EWL instance
 * Returns EWL_OK or EWL_ERROR
 * EWLRelease is called when the encoder instance is released */
i32 EWLRelease(const void *inst);

/* Reserve the HW resource for one codec instance
 * EWLReserveHw is called when beginning a frame encoding
 * The function may block until the resource is available.
 * Returns EWL_OK if the resource was successfully reserved for this instance
 * or EWL_ERROR if unable to reserve the resource. */
//    i32 EWLReserveHw(const void *inst);

/* Release the HW resource
 * EWLReleaseHw is called when the HW has finished the frame encoding.
 * The codec SW will continue the frame encoding but the HW can
 * be used by another codec.*/
//    void EWLReleaseHw(const void *inst);

/* Frame buffers memory */
//    i32 EWLMallocRefFrm(const void *inst, u32 size, VPULinearMem_t * info);
//    void EWLFreeRefFrm(const void *inst, VPULinearMem_t * info);

/* SW/HW shared memory */
//    i32 EWLMallocLinear(const void *inst, u32 size, VPULinearMem_t * info);
//    void VPUFreeLinear(const void *inst, VPULinearMem_t * info);

/* D-Cache coherence *//* Not in use currently */
//    void EWLDCacheRangeFlush(const void *instance, VPULinearMem_t * info);
//    void EWLDCacheRangeRefresh(const void *instance, VPULinearMem_t * info);

/* Write value to a HW register
 * All registers are written at once at the beginning of frame encoding
 * Offset is relative to the the HW ID register (#0) in bytes
 * Enable indicates when the HW is enabled. If shadow registers are used then
 * they must be flushed to the HW registers when enable is '1' before
 * writing the register that enables the HW */
void EWLWriteReg(const void *inst, u32 offset, u32 val);

/* Read and return the value of a HW register
 * The status register is read after every macroblock encoding by SW
 * The other registers which may be updated by the HW are read after
 * BUFFER_FULL or FRAME_READY interrupt
 * Offset is relative to the the HW ID register (#0) in bytes */
u32 EWLReadReg(const void *inst, u32 offset);

/* Writing all registers in one call *//*Not in use currently */
void EWLWriteRegAll(const void *inst, const u32 * table, u32 size);
/* Reading all registers in one call *//*Not in use currently */
void EWLReadRegAll(const void *inst, u32 * table, u32 size);

/* HW enable/disable. This will write <val> to register <offset> and by */
/* this enablig/disabling the hardware */
void EWLEnableHW(const void *inst, u32 offset, u32 val);
void EWLDisableHW(const void *inst, u32 offset, u32 val);



/* SW/SW shared memory handling */
void *EWLmalloc(u32 n);
//   void *EWLcalloc(u32 n, u32 s);
//    void EWLfree(void *p);
void *EWLmemcpy(void *d, const void *s, u32 n);
void *EWLmemset(void *d, i32 c, u32 n);

#ifdef __cplusplus
}
#endif
#endif /*__EWL_H__*/
