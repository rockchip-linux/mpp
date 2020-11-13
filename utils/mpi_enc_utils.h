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
#include "iniparser.h"

typedef struct MpiEncTestArgs_t {
    char                *file_input;
    char                *file_output;
    char                *file_cfg;
    dictionary          *cfg_ini;

    MppCodingType       type;
    MppFrameFormat      format;
    RK_S32              num_frames;
    RK_S32              loop_cnt;

    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;

    RK_S32              bps_target;
    RK_S32              bps_max;
    RK_S32              bps_min;
    RK_S32              rc_mode;

    RK_S32              fps_in_flex;
    RK_S32              fps_in_num;
    RK_S32              fps_in_den;
    RK_S32              fps_out_flex;
    RK_S32              fps_out_num;
    RK_S32              fps_out_den;

    RK_S32              gop_mode;
    RK_S32              gop_len;
    RK_S32              vi_len;

    MppEncHeaderMode    header_mode;

    MppEncSliceSplit    split;
} MpiEncTestArgs;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpi_enc_width_default_stride(RK_S32 width, MppFrameFormat fmt);

/*
 * gop_mode
 * 0     - default IPPPP gop
 * 1 ~ 3 - tsvc2 ~ tsvc4
 * >=  4 - smart gop mode
 */
MPP_RET mpi_enc_gen_ref_cfg(MppEncRefCfg ref, RK_S32 gop_mode);
MPP_RET mpi_enc_gen_smart_gop_ref_cfg(MppEncRefCfg ref, RK_S32 gop_len, RK_S32 vi_len);
MPP_RET mpi_enc_gen_osd_data(MppEncOSDData *osd_data, MppBufferGroup group,
                             RK_U32 width, RK_U32 height, RK_U32 frame_cnt);
MPP_RET mpi_enc_gen_osd_plt(MppEncOSDPlt *osd_plt, RK_U32 frame_cnt);

MpiEncTestArgs *mpi_enc_test_cmd_get(void);
MPP_RET mpi_enc_test_cmd_update_by_args(MpiEncTestArgs* cmd, int argc, char **argv);
MPP_RET mpi_enc_test_cmd_put(MpiEncTestArgs* cmd);

MPP_RET mpi_enc_test_cmd_show_opt(MpiEncTestArgs* cmd);
void mpi_enc_test_help(void);

#ifdef __cplusplus
}
#endif

#endif /*__MPI_ENC_UTILS_H__*/
