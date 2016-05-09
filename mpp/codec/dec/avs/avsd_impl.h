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

#ifndef __AVSD_IMPL_H__
#define __AVSD_IMPL_H__


#include "rk_type.h"

#include "mpp_err.h"
#include "mpp_packet.h"

#include "hal_task.h"



#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET lib_avsd_free(void *decoder);
void   *lib_avsd_malloc(void *decoder);
MPP_RET lib_prepare_one_frame(void *decoder, MppPacket pkt, HalDecTask *task);
MPP_RET lib_parse_one_frame(void *decoder, HalDecTask *task);
MPP_RET lib_init_one_frame(void *decoder, HalDecTask *task);
MPP_RET lib_decode_one_frame(void *decoder, HalDecTask *task);
MPP_RET lib_flush(void *decoder);
MPP_RET lib_reset(void *decoder);

MPP_RET nv12_copy_buffer(void *p_dec, RK_U8 *des);


#ifdef  __cplusplus
}
#endif

#endif /*__AVSD_IMPL_H__*/
