/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __MPP_IMPL_H__
#define __MPP_IMPL_H__

#include "rk_type.h"

#define MPP_PACKET_FLAG_EOS         (0x00000001)

/*
 * mpp_packet_imp structure
 *
 * data     : pointer
 * size     : total buffer size
 * offset   : valid data start offset
 * length   : valid data length
 * pts      : packet pts
 * dts      : packet dts
 */
typedef struct {
    void   *data;
    size_t  size;
    size_t  offset;
    size_t  length;

    RK_S64  pts;
    RK_S64  dts;

    RK_U32  flag;
} mpp_packet_impl;

#endif /*__MPP_IMPL_H__*/
