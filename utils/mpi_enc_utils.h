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

#ifndef MPI_ENC_UTILS_H
#define MPI_ENC_UTILS_H

#include <stdio.h>
#include "iniparser.h"
#include "utils.h"

#include "rk_mpi.h"
#include "rk_venc_kcfg.h"
#include "mpp_enc_args.h"

#include "camera_source.h"
#include "mpp_enc_roi_utils.h"

#define MPI_ENC_MAX_CHN 30

typedef struct MpiEncTestArgs_t {
    char                *file_input;
    char                *file_output;
    char                *file_cfg;
    dictionary          *cfg_ini;

    MppCodingType       type;
    MppCodingType       type_src;       /* for file source input */
    MppFrameFormat      format;
    RK_S32              frame_num;
    RK_S32              loop_cnt;
    RK_S32              nthreads;
    RK_S32              frm_step;

    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;

    /* -rc */
    RK_S32              rc_mode;

    /* -bps */
    RK_S32              bps_target;
    RK_S32              bps_max;
    RK_S32              bps_min;

    /* -fps */
    RK_S32              fps_in_flex;
    RK_S32              fps_in_num;
    RK_S32              fps_in_den;
    RK_S32              fps_out_flex;
    RK_S32              fps_out_num;
    RK_S32              fps_out_den;

    /* -qc */
    RK_S32              qp_init;
    RK_S32              qp_min;
    RK_S32              qp_max;
    RK_S32              qp_min_i;
    RK_S32              qp_max_i;

    /* -fqc */
    RK_S32              fqp_min_i;
    RK_S32              fqp_min_p;
    RK_S32              fqp_max_i;
    RK_S32              fqp_max_p;

    /* -g gop mode */
    RK_S32              gop_mode;
    RK_S32              gop_len;
    RK_S32              vi_len;

    /* -sm scene_mode */
    RK_S32              scene_mode;
    RK_S32              rc_container;
    RK_S32              bias_i;
    RK_S32              bias_p;

    /* -qpdd cu_qp_delta_depth */
    RK_S32              cu_qp_delta_depth;
    RK_S32              anti_flicker_str;
    RK_S32              atf_str;
    RK_S32              atr_str_i;
    RK_S32              atr_str_p;
    RK_S32              atl_str;
    RK_S32              sao_str_i;
    RK_S32              sao_str_p;
    RK_S32              lambda_idx_p;
    RK_S32              lambda_idx_i;
    RK_S32              speed;
    /* -dbe deblur enable flag
     * -dbs deblur strength
     */
    RK_S32              deblur_en;
    RK_S32              deblur_str;

    /* -v q runtime log disable flag */
    RK_U32              quiet;
    /* -v f runtime fps log flag */
    RK_U32              trace_fps;
    FpsCalc             fps;
    RK_U32              psnr_en;
    RK_U32              ssim_en;
    char                *file_slt;
    RK_U32              kmpp_en;

    RK_U32              split_mode;
    RK_U32              split_arg;
    RK_U32              split_out;

    RK_U32              osd_enable;
    RK_U32              osd_mode;

    RK_U32              user_data_enable;
    RK_U32              roi_enable;
    RK_U32              roi_jpeg_enable;
    RK_U32              jpeg_osd_case;

    RK_U32              mirroring;
    RK_U32              rotation;
    RK_U32              flip;

    RK_U32              constraint_set;
    RK_U32              sei_mode;
} MpiEncTestArgs;

typedef struct MppEncTestObjSet_t {
    MppEncArgs          cmd_obj;
    MpiEncTestArgs      *cmd;
    MppEncCfg           cfg_obj;
} MppEncTestObjSet;

typedef struct {
    // base flow context
    MppCtx              ctx;
    MppApi              *mpi;
    RK_S32              chn;

    // global flow control flag
    RK_U32              frm_eos;
    RK_U32              pkt_eos;
    RK_U32              frm_pkt_cnt;
    RK_S32              frame_num;
    RK_S32              frm_cnt_in;
    RK_S32              frm_cnt_out;
    RK_S32              frm_step;

    RK_U64              stream_size;
    /* end of encoding flag when set quit the loop */
    volatile RK_U32     loop_end;

    // src and dst
    FILE                *fp_input;
    FILE                *fp_output[MPI_ENC_MAX_CHN];
    FILE                *fp_verify;

    /* encoder config set */
    MppEncCfg           cfg;
    MppEncOSDPltCfg     osd_plt_cfg;
    MppEncOSDPlt        osd_plt;
    MppEncOSDData       osd_data;
    RoiRegionCfg        roi_region;
    MppEncROICfg        roi_cfg;
    MppJpegROICfg       roi_jpeg_cfg;

    // input / output
    MppBufferGroup      buf_grp;
    MppEncHeaderMode    header_mode;

    // paramter for resource malloc
    RK_U32              width;
    RK_U32              height;
    RK_U32              hor_stride;
    RK_U32              ver_stride;
    MppFrameFormat      fmt;
    MppCodingType       type;
    RK_S32              loop_times;
    CamSource           *cam_ctx;
    MppEncRoiCtx        roi_ctx;

    MppVencKcfg         init_kcfg;

    // resources
    size_t              header_size;
    size_t              frame_size;
    /* NOTE: packet buffer may overflow */
    size_t              packet_size;

    RK_S64              first_frm;
    RK_S64              first_pkt;
    RK_S64              last_pkt;

    MppEncOSDData3      osd_data3;
    KmppBuffer          osd_buffer;
    RK_U8               *osd_pattern;
} MpiEncTestData;

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

MPP_RET mpi_enc_test_objset_update_by_args(MppEncTestObjSet *obj_set, int argc, char **argv, const char *module_tag);
MPP_RET mpi_enc_test_cmd_put(MppEncTestObjSet* obj_set);

MPP_RET mpi_enc_ctx_init(MpiEncTestData *p, MpiEncTestArgs *cmd, RK_S32 chn);
MPP_RET mpi_enc_ctx_deinit(MpiEncTestData *p);
MPP_RET mpi_enc_cfg_setup(MpiEncTestData *p, MpiEncTestArgs *cmd, MppEncCfg cfg_obj);

MPP_RET mpi_enc_test_objset_get(MppEncTestObjSet **obj_set);
MPP_RET mpi_enc_test_objset_put(MppEncTestObjSet *obj_set);

#ifdef __cplusplus
}
#endif

#endif /* MPI_ENC_UTILS_H */
