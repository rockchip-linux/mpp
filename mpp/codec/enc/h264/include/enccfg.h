/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Rockchip Products .                             --
--                                                                            --
--                   (C) COPYRIGHT 2014 ROCKCHIP PRODUCTS                     --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Encoder common configuration parameters
--
--------------------------------------------------------------------------------
--
--
--
------------------------------------------------------------------------------*/
#ifndef __ENCCFG_H__
#define __ENCCFG_H__

#include "basetype.h"

/* Here is defined the default values for the encoder build-time configuration.
 * You can override these settings by defining the values as compiler flags
 * in the Makefile.
 */

/* The input image's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC input YUV
 * 1 = 64-bit endianess */
#ifndef ENC6820_INPUT_SWAP_32_YUV
#define ENC6820_INPUT_SWAP_32_YUV                   1
#endif

/* The input image's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input YUV
 */
#ifndef ENC6820_INPUT_SWAP_16_YUV
#define ENC6820_INPUT_SWAP_16_YUV                   1
#endif

/* The input image's 8-bit swap: 0 or 1
 * This defines the byte endianess of the ASIC input YUV
 */
#ifndef ENC6820_INPUT_SWAP_8_YUV
#define ENC6820_INPUT_SWAP_8_YUV                    1
#endif

/* The input image's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC input RGB16
 * 1 = 64-bit endianess */
#ifndef ENC6820_INPUT_SWAP_32_RGB16
#define ENC6820_INPUT_SWAP_32_RGB16                 1
#endif

/* The input image's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input RGB16
 */
#ifndef ENC6820_INPUT_SWAP_16_RGB16
#define ENC6820_INPUT_SWAP_16_RGB16                 1
#endif

/* The input image's byte swap: 0 or 1
 * This defines the byte endianess of the ASIC input RGB16
 */
#ifndef ENC6820_INPUT_SWAP_8_RGB16
#define ENC6820_INPUT_SWAP_8_RGB16                  0
#endif

/* The input image's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC input RGB32
 * 1 = 64-bit endianess */
#ifndef ENC6820_INPUT_SWAP_32_RGB32
#define ENC6820_INPUT_SWAP_32_RGB32                 1   //wjm   wjm
#endif

/* The input image's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input RGB32
 */
#ifndef ENC6820_INPUT_SWAP_16_RGB32
#define ENC6820_INPUT_SWAP_16_RGB32                 0
#endif

/* The input image's byte swap: 0 or 1
 * This defines the byte endianess of the ASIC input RGB32
 */
#ifndef ENC6820_INPUT_SWAP_8_RGB32
#define ENC6820_INPUT_SWAP_8_RGB32                  0
#endif

/* ENC6820_OUTPUT_SWAP_XX define the byte endianess of the ASIC output data.
 * This MUST be configured to be the same as the native system endianess,
 * because the control software relies on system endianess when reading
 * the data from the memory. */

/* The output data's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC output data
 * 1 = 64-bit endianess */
#ifndef ENC6820_OUTPUT_SWAP_32
#define ENC6820_OUTPUT_SWAP_32                      1
#endif

/* The output data's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC output data.
 */
#ifndef ENC6820_OUTPUT_SWAP_16
#define ENC6820_OUTPUT_SWAP_16                      1
#endif

/* The output data's 8-bit swap: 0 or 1
 * This defines the byte endianess of the ASIC output data.
 */
#ifndef ENC6820_OUTPUT_SWAP_8
#define ENC6820_OUTPUT_SWAP_8                       1
#endif

/* ASIC interrupt enable.
 * This enables/disables the ASIC to generate interrupts
 * If this is '1', the EWL must poll the registers to find out
 * when the HW is ready.
 */
#ifndef ENC6820_IRQ_DISABLE
#define ENC6820_IRQ_DISABLE                         0
#endif

/* ASIC bus interface configuration values                  */
/* DO NOT CHANGE IF NOT FAMILIAR WITH THE CONCEPTS INVOLVED */

/* Burst length. This sets the maximum length of a single ASIC burst in addresses.
 * Allowed values are:
 *          AHB {0, 4, 8, 16} ( 0 means incremental burst type INCR)
 *          OCP [1,63]
 *          AXI [1,16]
 */
#ifndef ENC6820_BURST_LENGTH
#define ENC6820_BURST_LENGTH                               16
#endif

/* INCR type burst mode                                                       */
/* 0 - enable INCR type bursts                                                */
/* 1 - disable INCR type and use SINGLE instead                               */
#ifndef ENC6820_BURST_INCR_TYPE_ENABLED
#define ENC6820_BURST_INCR_TYPE_ENABLED                    0
#endif

/* Data discard mode. When enabled read bursts of length 2 or 3 are converted */
/* to BURST4 and  useless data is discarded. Otherwise use INCR type for that */
/* kind  of read bursts */
/* 0 - disable data discard                                                   */
/* 1 - enable data discard                                                    */
#ifndef ENC6820_BURST_DATA_DISCARD_ENABLED
#define ENC6820_BURST_DATA_DISCARD_ENABLED                 0
#endif

/* AXI bus read and write ID values used by HW. 0 - 255 */
#ifndef ENC6820_AXI_READ_ID
#define ENC6820_AXI_READ_ID                                0
#endif

#ifndef ENC6820_AXI_WRITE_ID
#define ENC6820_AXI_WRITE_ID                               0
#endif

/* End of "ASIC bus interface configuration values"                           */

/* ASIC internal clock gating control. 0 - disabled, 1 - enabled              */
#ifndef ENC6820_ASIC_CLOCK_GATING_ENABLED
#define ENC6820_ASIC_CLOCK_GATING_ENABLED                  0
#endif

/* ASIC timeout interrupt enable/disable                                      */
#ifndef ENC6820_TIMEOUT_INTERRUPT
#define ENC6820_TIMEOUT_INTERRUPT                          1
#endif

#define ENC6820_REGISTERS  184

#endif
