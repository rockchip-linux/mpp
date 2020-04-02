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

#ifndef __MPI_ENC_UTILS_H__
#define __MPI_ENC_UTILS_H__

#include <stdio.h>

#include "rk_venc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpi_enc_gen_osd_data(MppEncOSDData *osd_data, MppBuffer osd_buf, RK_U32 frame_cnt);
MPP_RET mpi_enc_gen_osd_plt(MppEncOSDPlt *osd_plt, RK_U32 *table);

#ifdef __cplusplus
}
#endif

#endif /*__MPI_ENC_UTILS_H__*/
