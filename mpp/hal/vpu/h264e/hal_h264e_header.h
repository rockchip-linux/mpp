/*
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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

#ifndef _HAL_H264E_HEADER_H_
#define _HAL_H264E_HEADER_H_

#include <stdio.h>
#include <string.h>

#include "hal_h264e_com.h"

MPP_RET h264e_vpu_init_extra_info(void *extra_info);
MPP_RET h264e_vpu_deinit_extra_info(void *extra_info);
MPP_RET h264e_vpu_set_extra_info(H264eHalContext *ctx);

MPP_RET h264e_vpu_free_buffers(H264eHalContext *ctx);
MPP_RET h264e_vpu_allocate_buffers(H264eHalContext *ctx);

#endif
