/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Basic type definitions.
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: basetype.h,v $
--  $Revision: 1.3 $
--  $Date: 2007/11/19 08:26:14 $
--
------------------------------------------------------------------------------*/

#ifndef __BASETYPE_H__
#define __BASETYPE_H__

#if defined( __linux__ ) || defined( WIN32 )
#include <stddef.h>
#endif

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

typedef unsigned char u8;
typedef signed char i8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short i16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int i32;
typedef signed int s32;

/* SW decoder 16 bits types */
#if defined(VC1SWDEC_16BIT) || defined(MP4ENC_ARM11)
typedef unsigned short u16x;
typedef signed short i16x;
#else
typedef unsigned int u16x;
typedef signed int i16x;
#endif

#endif /* __BASETYPE_H__ */
