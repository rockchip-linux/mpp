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

#ifndef __RK_TYPE_H__
#define __RK_TYPE_H__

#include <stddef.h>

#if defined(_WIN32) && !defined(__MINGW32CE__)

typedef unsigned char           RK_U8;
typedef unsigned short          RK_U16;
typedef unsigned int            RK_U32;
typedef unsigned long           RK_ULONG;
typedef unsigned __int64        RK_U64;

typedef signed char             RK_S8;
typedef signed short            RK_S16;
typedef signed int              RK_S32;
typedef signed long             RK_LONG;
typedef signed __int64          RK_S64;

#else

typedef unsigned char           RK_U8;
typedef unsigned short          RK_U16;
typedef unsigned int            RK_U32;
typedef unsigned long           RK_ULONG;
typedef unsigned long long int  RK_U64;


typedef signed char             RK_S8;
typedef signed short            RK_S16;
typedef signed int              RK_S32;
typedef signed long             RK_LONG;
typedef signed long long int    RK_S64;

#endif

#ifndef MODULE_TAG
#define MODULE_TAG              NULL
#endif

/*
 * All external interface object list here.
 * The interface object is defined as void * for expandability
 * The cross include between these objects will introduce extra
 * compiling difficulty. So we move them together in this header.
 *
 * Object interface header list:
 *
 * MppCtx           - rk_mpi.h
 * MppParam         - rk_mpi.h
 *
 * MppFrame         - mpp_frame.h
 * MppPacket        - mpp_packet.h
 *
 * MppBuffer        - mpp_buffer.h
 * MppBufferGroup   - mpp_buffer.h
 *
 * MppTask          - mpp_task.h
 * MppMeta          - mpp_meta.h
 */

typedef void* MppCtx;
typedef void* MppParam;

typedef void* MppFrame;
typedef void* MppPacket;

typedef void* MppBuffer;
typedef void* MppBufferGroup;

typedef void* MppTask;
typedef void* MppMeta;

#endif /*__RK_TYPE_H__*/
