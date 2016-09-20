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

#define MODULE_TAG "h264e_hal_test"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef RKPLATFORM
#include <sys/time.h>
#endif

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_frame.h"

#include "hal_h264e_api.h"
#include "hal_h264e.h"
#include "hal_h264e_vpu.h"
#include "hal_h264e_rkv.h"


static HalDeviceId test_device_id = HAL_RKVENC;

#define MAX_FRAME_LUMA_SIZE  (4096*2304)
#define MAX_FRAME_TOTAL_SIZE  (MAX_FRAME_LUMA_SIZE*3/2)
#define GET_U32(val)  ((RK_U32)(*((RK_U32 *)&(val))))
//#define GET_U32(val)  ((RK_U32)(val))

#define H264E_HAL_FSCAN(fp, fmt, dst)   \
    do {                                \
        fscanf(fp, fmt, &data);         \
        dst = data;                     \
        fgets(temp, 512, fp);           \
    }while(0)

//golden data from on2 project
FILE *fp_golden_syntax_in = NULL;
FILE *fp_mpp_syntax_in = NULL;
FILE *fp_mpp_reg_in = NULL;
FILE *fp_mpp_reg_out = NULL;
FILE *fp_h264e_yuv_in = NULL;
FILE *fp_mpp_strm_out = NULL;
FILE *fp_mpp_feedback = NULL;
FILE *fp_mpp_yuv_in = NULL;
FILE *fp_mpp_wr2hw_reg_in = NULL;

RK_U32 g_frame_cnt = 0;
RK_U32 g_frame_read_cnt = 0;
typedef struct h264e_hal_test_cfg_t {
    RK_U32 hw_mode;
    char input_syntax_file_path[256];
    char input_yuv_file_path[256];
    RK_U32 pic_width;
    RK_U32 pic_height;
    RK_U32 src_format;
    RK_U32 num_frames;

    h264e_hal_rkv_coveragetest_cfg test;
} h264e_hal_test_cfg;

static RK_U32 h264e_rkv_revert_csp(h264e_hal_rkv_csp_info csp_info)
{
    h264e_hal_rkv_csp fmt = (h264e_hal_rkv_csp)csp_info.fmt;
    RK_U32 cswap = csp_info.cswap;
    RK_U32 aswap = csp_info.aswap;
    MppFrameFormat dst_fmt;

    switch (fmt) {
    case H264E_RKV_CSP_YUV420P: {
        dst_fmt = MPP_FMT_YUV420P;
        break;
    }
    case H264E_RKV_CSP_YUV420SP: {
        dst_fmt = cswap ? MPP_FMT_YUV420SP_VU : MPP_FMT_YUV420SP;
        break;
    }
    case H264E_RKV_CSP_YUV422P: {
        dst_fmt = MPP_FMT_YUV422P;
        break;
    }
    case H264E_RKV_CSP_YUV422SP: {
        dst_fmt = cswap ? MPP_FMT_YUV422SP_VU : MPP_FMT_YUV422SP;
        break;
    }
    case H264E_RKV_CSP_YUYV422: {
        dst_fmt = MPP_FMT_YUV422_YUYV;
        break;
    }
    case H264E_RKV_CSP_UYVY422: {
        dst_fmt = MPP_FMT_YUV422_UYVY;
        break;
    }
    case H264E_RKV_CSP_BGR565: {
        dst_fmt = cswap ? MPP_FMT_RGB565 : MPP_FMT_BGR565;
        break;
    }
    case H264E_RKV_CSP_BGR888: {
        dst_fmt = cswap ? MPP_FMT_RGB888 : MPP_FMT_BGR888;
        break;
    }
    case H264E_RKV_CSP_BGRA8888: {
        if (aswap)
            dst_fmt = cswap ? MPP_FMT_ABGR8888 : MPP_FMT_ARGB8888;
        else
            dst_fmt = MPP_FMT_BUTT;

        break;
    }
    default: {
        h264e_hal_log_err("invalid csp_info.fmt %d, csp_info.cswap %d, csp_info.aswap %d",
                          csp_info.fmt, csp_info.cswap, csp_info.aswap);
        dst_fmt = MPP_FMT_BUTT;
    }
    }

    if (dst_fmt == MPP_FMT_BUTT) {
        h264e_hal_log_err("revert_csp error, src_fmt %d, dst_fmt %d", fmt, dst_fmt);
    }

    return (RK_U32)dst_fmt;
}

static RK_U32 h264e_vpu_revert_csp(RK_U32 csp)
{
    h264e_hal_vpu_csp fmt = (h264e_hal_vpu_csp)csp;
    MppFrameFormat dst_fmt;

    switch (fmt) {
    case H264E_VPU_CSP_YUV420P: {
        dst_fmt = MPP_FMT_YUV420P;
        break;
    }
    case H264E_VPU_CSP_YUV420SP: {
        dst_fmt = MPP_FMT_YUV420SP;
        break;
    }
    case H264E_VPU_CSP_YUYV422: {
        dst_fmt = MPP_FMT_YUV422_YUYV;
        break;
    }
    case H264E_VPU_CSP_UYVY422: {
        dst_fmt = MPP_FMT_YUV422_UYVY;
        break;
    }
    case H264E_VPU_CSP_RGB565: {
        dst_fmt = MPP_FMT_RGB565;
        break;
    }
    case H264E_VPU_CSP_BGR565: {
        dst_fmt = MPP_FMT_BGR565;
        break;
    }
    case H264E_VPU_CSP_RGB555: {
        dst_fmt = MPP_FMT_RGB555;
        break;
    }
    case H264E_VPU_CSP_BGR555: {
        dst_fmt = MPP_FMT_BGR555;
        break;
    }
    case H264E_VPU_CSP_RGB444: {
        dst_fmt = MPP_FMT_RGB444;
        break;
    }
    case H264E_VPU_CSP_BGR444: {
        dst_fmt = MPP_FMT_BGR444;
        break;
    }
    case H264E_VPU_CSP_RGB888: {
        dst_fmt = MPP_FMT_RGB888;
        break;
    }
    case H264E_VPU_CSP_BGR888: {
        dst_fmt = MPP_FMT_BGR888;
        break;
    }
    case H264E_VPU_CSP_RGB101010: {
        dst_fmt = MPP_FMT_RGB101010;
        break;
    }
    case H264E_VPU_CSP_BGR101010: {
        dst_fmt = MPP_FMT_BGR101010;
        break;
    }
    default: {
        h264e_hal_log_err("invalid csp %d", csp);
        dst_fmt = MPP_FMT_BUTT;
    }
    }

    return (RK_U32)dst_fmt;
}


static MPP_RET h264e_rkv_test_open_files(h264e_hal_test_cfg test_cfg)
{
    char base_path[512];
    char full_path[512];
    strcpy(base_path, "/sdcard/h264e_data/");

    sprintf(full_path, "%s", test_cfg.input_yuv_file_path);
    fp_h264e_yuv_in = fopen(full_path, "rb");
    if (!fp_h264e_yuv_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

#if !RKV_H264E_SDK_TEST
    sprintf(full_path, "%s", test_cfg.input_syntax_file_path);
    fp_golden_syntax_in = fopen(full_path, "rb");
    if (!fp_golden_syntax_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }
#endif

#if 0
    sprintf(full_path, "%s%s", base_path, "mpp_yuv_in.yuv");
    fp_mpp_yuv_in = fopen(full_path, "wb");
    if (!fp_mpp_yuv_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }
#endif

    return MPP_OK;
}


static MPP_RET h264e_vpu_test_open_files(h264e_hal_test_cfg test_cfg)
{
    char base_path[512];
    char full_path[512];
    strcpy(base_path, "/sdcard/h264e_data/");

    sprintf(full_path, "%s", test_cfg.input_yuv_file_path);
    fp_h264e_yuv_in = fopen(full_path, "rb");
    if (!fp_h264e_yuv_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    sprintf(full_path, "%s", test_cfg.input_syntax_file_path);
    fp_golden_syntax_in = fopen(full_path, "rb");
    if (!fp_golden_syntax_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    sprintf(full_path, "%s%s", base_path, "mpp_yuv_in.yuv");
    fp_mpp_yuv_in = fopen(full_path, "wb");
    if (!fp_mpp_yuv_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    return MPP_OK;
}



static MPP_RET h264e_test_close_files()
{
    if (fp_h264e_yuv_in) {
        fclose(fp_h264e_yuv_in);
        fp_h264e_yuv_in = NULL;
    }

    if (fp_golden_syntax_in) {
        fclose(fp_golden_syntax_in);
        fp_golden_syntax_in = NULL;
    }

    if (fp_mpp_syntax_in) {
        fclose(fp_mpp_syntax_in);
        fp_mpp_syntax_in = NULL;
    }


    if (fp_mpp_yuv_in) {
        fclose(fp_mpp_yuv_in);
        fp_mpp_yuv_in = NULL;
    }

    if (fp_mpp_reg_in) {
        fclose(fp_mpp_reg_in);
        fp_mpp_reg_in = NULL;
    }

    if (fp_mpp_reg_out) {
        fclose(fp_mpp_reg_out);
        fp_mpp_reg_out = NULL;
    }

    if (fp_mpp_strm_out) {
        fclose(fp_mpp_strm_out);
        fp_mpp_strm_out = NULL;
    }

    if (fp_mpp_feedback) {
        fclose(fp_mpp_feedback);
        fp_mpp_feedback = NULL;
    }

    if (fp_mpp_wr2hw_reg_in) {
        fclose(fp_mpp_wr2hw_reg_in);
        fp_mpp_wr2hw_reg_in = NULL;
    }

    return MPP_OK;
}

static MPP_RET get_rkv_h264e_yuv_in_frame(h264e_syntax *syn, MppBuffer *hw_buf)
{
    RK_U32 read_ret = 0;
    RK_U32 frame_luma_size = syn->pic_luma_width * syn->pic_luma_height;
    RK_U32 frame_size = frame_luma_size * 3 / 2;
    RK_U8 *hw_buf_ptr = (RK_U8 *)mpp_buffer_get_ptr(hw_buf[g_frame_read_cnt % RKV_H264E_LINKTABLE_FRAME_NUM]);;
    mpp_assert(fp_h264e_yuv_in);

    read_ret = (RK_U32)fread(hw_buf_ptr, 1, frame_size, fp_h264e_yuv_in);
    if (read_ret == 0) {
        mpp_log("yuv file end, nothing is read in %d frame", g_frame_read_cnt);
        return MPP_EOS_STREAM_REACHED;
    } else if (read_ret != frame_size) {
        mpp_err("read yuv frame %d error, needed:%d, actual:%d", g_frame_read_cnt, frame_size, read_ret);
        return MPP_NOK;
    } else {
        mpp_log("read frame %d size %d successfully", g_frame_read_cnt, read_ret);
    }

    if (fp_mpp_yuv_in) {
        fwrite(hw_buf_ptr, 1, frame_size, fp_mpp_yuv_in);
    }

    return MPP_OK;
}

static MPP_RET get_h264e_yuv_in_one_frame(RK_U8 *sw_buf, h264e_syntax *syn, MppBuffer hw_buf)
{
    RK_U32 read_ret = 0;
    RK_U32 frame_luma_size = syn->pic_luma_width * syn->pic_luma_height;
    RK_U32 frame_size = frame_luma_size * 3 / 2;
    mpp_assert(fp_h264e_yuv_in);

    //TODO: remove sw_buf, read & write fd ptr directly
    read_ret = (RK_U32)fread(sw_buf, 1, frame_size, fp_h264e_yuv_in);
    if (read_ret == 0) {
        mpp_log("yuv file end, nothing is read in frame %d", g_frame_cnt);
        return MPP_EOS_STREAM_REACHED;
    } else if (read_ret != frame_size) {
        mpp_err("read yuv one frame error, needed:%d, actual:%d", frame_size, read_ret);
        return MPP_NOK;
    } else {
        mpp_log("read frame %d size %d successfully", g_frame_cnt, frame_size);
    }


    mpp_buffer_write(hw_buf, 0, sw_buf,  frame_size);

    if (fp_mpp_yuv_in) {
        memcpy(sw_buf, mpp_buffer_get_ptr(hw_buf), frame_size);
        fwrite(sw_buf, 1, frame_luma_size, fp_mpp_yuv_in);
    }
    return MPP_OK;
}

static MPP_RET get_vpu_syntax_in(h264e_syntax *syn, MppBuffer hw_in_buf, MppBuffer hw_output_strm_buf, RK_U32 frame_luma_size)
{
    RK_S32 k = 0;
    mpp_assert(fp_golden_syntax_in);
    memset(syn, 0, sizeof(h264e_syntax));

    if (fp_golden_syntax_in) {
        char temp[512] = {0};
        RK_S32 data = 0;
        if (!fgets(temp, 512, fp_golden_syntax_in))
            return MPP_EOS_STREAM_REACHED;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->frame_coding_type = data;


        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->pic_init_qp = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->slice_alpha_offset = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->slice_beta_offset = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->chroma_qp_index_offset = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->filter_disable = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->idr_pic_id = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->pps_id = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->frame_num = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->slice_size_mb_rows = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->h264_inter4x4_disabled = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->enable_cabac = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->transform8x8_mode = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->cabac_init_idc = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->qp = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->mad_qp_delta = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->mad_threshold = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->qp_min = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->qp_max = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->cp_distance_mbs = data;

        for (k = 0; k < 10; k++) {
            fscanf(fp_golden_syntax_in, "%d", &data);
            fgets(temp, 512, fp_golden_syntax_in);
            syn->cp_target[k] = data;
        }

        for (k = 0; k < 7; k++) {
            fscanf(fp_golden_syntax_in, "%d", &data);
            fgets(temp, 512, fp_golden_syntax_in);
            syn->target_error[k] = data;
        }
        for (k = 0; k < 7; k++) {
            fscanf(fp_golden_syntax_in, "%d", &data);
            fgets(temp, 512, fp_golden_syntax_in);
            syn->delta_qp[k] = data;
        }

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->output_strm_limit_size = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->pic_luma_width = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->pic_luma_height = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->input_image_format = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->color_conversion_coeff_a = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->color_conversion_coeff_b = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->color_conversion_coeff_c = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->color_conversion_coeff_e = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->color_conversion_coeff_f = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);

        fgets(temp, 512, fp_golden_syntax_in);
    }

    if (hw_in_buf)
        syn->input_luma_addr = mpp_buffer_get_fd(hw_in_buf);
    syn->input_cb_addr = syn->input_luma_addr | (frame_luma_size << 10);
    syn->input_cr_addr = syn->input_cb_addr | ((frame_luma_size / 4) << 10);

    if (hw_output_strm_buf)
        syn->output_strm_addr = mpp_buffer_get_fd(hw_output_strm_buf);

    /* adjust */
    syn->input_image_format = h264e_vpu_revert_csp(syn->input_image_format);

    return MPP_OK;
}

#if 0
static void dump_rkv_mpp_dbg_info(h264e_hal_rkv_dbg_info *info, h264e_syntax *syn)
{
    if (fp_mpp_syntax_in) {
        RK_S32 k = 0;
        FILE *fp = fp_mpp_syntax_in;
        fprintf(fp, "#FRAME %d\n", g_frame_read_cnt);

        fprintf(fp, "%-16d %s\n", syn->pic_luma_width, "pic_luma_width");
        fprintf(fp, "%-16d %s\n", syn->pic_luma_height, "pic_luma_height");
        fprintf(fp, "%-16d %s\n", syn->level_idc, "level_idc");
        fprintf(fp, "%-16d %s\n", syn->profile_idc, "profile_idc");
        fprintf(fp, "%-16d %s\n", syn->frame_coding_type, "frame_coding_type");

        fprintf(fp, "%-16d %s\n", info->swreg02.lkt_num, "swreg02.lkt_num");
        fprintf(fp, "%-16d %s\n", info->swreg02.rkvenc_cmd, "swreg02.rkvenc_cmd");
        fprintf(fp, "%-16d %s\n", info->swreg02.enc_cke, "swreg02.enc_cke");

        fprintf(fp, "%-16d %s\n", info->swreg04.lkt_addr, "swreg04.lkt_addr");

        fprintf(fp, "%-16d %s\n", info->swreg05.ofe_fnsh, "swreg05.ofe_fnsh");
        fprintf(fp, "%-16d %s\n", info->swreg05.lkt_fnsh, "swreg05.lkt_fnsh");
        fprintf(fp, "%-16d %s\n", info->swreg05.clr_fnsh, "swreg05.clr_fnsh");
        fprintf(fp, "%-16d %s\n", info->swreg05.ose_fnsh, "swreg05.ose_fnsh");
        fprintf(fp, "%-16d %s\n", info->swreg05.bs_ovflr, "swreg05.bs_ovflr");
        fprintf(fp, "%-16d %s\n", info->swreg05.brsp_ful, "swreg05.brsp_ful");
        fprintf(fp, "%-16d %s\n", info->swreg05.brsp_err, "swreg05.brsp_err");
        fprintf(fp, "%-16d %s\n", info->swreg05.rrsp_err, "swreg05.rrsp_err");
        fprintf(fp, "%-16d %s\n", info->swreg05.tmt_err, "swreg05.tmt_err");

        fprintf(fp, "%-16d %s\n", info->swreg10.roi_enc, "swreg10.roi_enc");
        fprintf(fp, "%-16d %s\n", info->swreg10.cur_frm_ref, "swreg10.cur_frm_ref");
        fprintf(fp, "%-16d %s\n", info->swreg10.mei_stor, "swreg10.mei_stor");
        fprintf(fp, "%-16d %s\n", info->swreg10.bs_scp, "swreg10.bs_scp");
        fprintf(fp, "%-16d %s\n", info->swreg10.pic_qp, "swreg10.pic_qp");
        fprintf(fp, "%-16d %s\n", info->swreg10.slice_int, "swreg10.slice_int");
        fprintf(fp, "%-16d %s\n", info->swreg10.node_int, "swreg10.node_int");

        fprintf(fp, "%-16d %s\n", info->swreg11.ppln_enc_lmt, "swreg11.ppln_enc_lmt");
        fprintf(fp, "%-16d %s\n", info->swreg11.rfp_load_thrd, "swreg11.rfp_load_thrd");

        fprintf(fp, "%-16d %s\n", info->swreg12.src_bus_edin, "swreg12.src_bus_edin");


        fprintf(fp, "%-16d %s\n", info->swreg13.axi_brsp_cke, "swreg13.axi_brsp_cke");

        fprintf(fp, "%-16d %s\n", info->swreg14.src_aswap, "swreg14.src_aswap");
        fprintf(fp, "%-16d %s\n", info->swreg14.src_cswap, "swreg14.src_cswap");
        fprintf(fp, "%-16d %s\n", info->swreg14.src_cfmt, "swreg14.src_cfmt");
        fprintf(fp, "%-16d %s\n", info->swreg14.src_clip_dis, "swreg14.src_clip_dis");

        fprintf(fp, "%-16d %s\n", info->swreg15.wght_b2y, "swreg15.wght_b2y");
        fprintf(fp, "%-16d %s\n", info->swreg15.wght_g2y, "swreg15.wght_g2y");
        fprintf(fp, "%-16d %s\n", info->swreg15.wght_r2y, "swreg15.wght_r2y");

        fprintf(fp, "%-16d %s\n", info->swreg16.wght_b2u, "swreg16.wght_b2u");
        fprintf(fp, "%-16d %s\n", info->swreg16.wght_g2u, "swreg16.wght_g2u");
        fprintf(fp, "%-16d %s\n", info->swreg16.wght_r2u, "swreg16.wght_r2u");

        fprintf(fp, "%-16d %s\n", info->swreg17.wght_b2v, "swreg17.wght_b2v");
        fprintf(fp, "%-16d %s\n", info->swreg17.wght_g2v, "swreg17.wght_g2v");
        fprintf(fp, "%-16d %s\n", info->swreg17.wght_r2v, "swreg17.wght_r2v");

        fprintf(fp, "%-16d %s\n", info->swreg18.ofst_rgb2v, "swreg18.ofst_rgb2v");
        fprintf(fp, "%-16d %s\n", info->swreg18.ofst_rgb2u, "swreg18.ofst_rgb2u");
        fprintf(fp, "%-16d %s\n", info->swreg18.ofst_rgb2y, "swreg18.ofst_rgb2y");

        fprintf(fp, "%-16d %s\n", info->swreg19.src_tfltr, "swreg19.src_tfltr");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_tfltr_we, "swreg19.src_tfltr_we");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_tfltr_bw, "swreg19.src_tfltr_bw");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_sfltr, "swreg19.src_sfltr");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_mfltr_thrd, "swreg19.src_mfltr_thrd");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_mfltr_y, "swreg19.src_mfltr_y");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_mfltr_c, "swreg19.src_mfltr_c");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_bfltr_strg, "swreg19.src_bfltr_strg");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_bfltr, "swreg19.src_bfltr");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_mbflt_odr, "swreg19.src_mbflt_odr");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_matf_y, "swreg19.src_matf_y");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_matf_c, "swreg19.src_matf_c");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_shp_y, "swreg19.src_shp_y");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_shp_c, "swreg19.src_shp_c");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_shp_div, "swreg19.src_shp_div");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_shp_thld, "swreg19.src_shp_thld");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_mirr, "swreg19.src_mirr");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_rot, "swreg19.src_rot");
        fprintf(fp, "%-16d %s\n", info->swreg19.src_matf_itsy, "swreg19.src_matf_itsy");

        fprintf(fp, "%-16d %s\n", info->swreg20.tfltr_thld_y, "swreg20.tfltr_thld_y");
        fprintf(fp, "%-16d %s\n", info->swreg20.tfltr_thld_c, "swreg20.tfltr_thld_c");

        for (k = 0; k < 5; k++)
            fprintf(fp, "%-16d swreg21_scr_stbl[%d]\n", info->swreg21_scr_stbl[k], k);

        for (k = 0; k < 40; k++)
            fprintf(fp, "%-16d swreg22_h3d_tbl[%d]\n", info->swreg22_h3d_tbl[k], k);

        fprintf(fp, "%-16d %s\n", info->swreg23.src_ystrid, "swreg23.src_ystrid");
        fprintf(fp, "%-16d %s\n", info->swreg23.src_cstrid, "swreg23.src_cstrid");

        fprintf(fp, "%#-16x %s\n", info->addr_cfg.adr_srcy, "addr_cfg.adr_srcy");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.adr_srcu, "addr_cfg.adr_srcu");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.adr_srcv, "addr_cfg.adr_srcv");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.ctuc_addr, "addr_cfg.ctuc_addr");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.rfpw_addr, "addr_cfg.rfpw_addr");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.rfpr_addr, "addr_cfg.rfpr_addr");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.dspw_addr, "addr_cfg.dspw_addr");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.dspr_addr, "addr_cfg.dspr_addr");
        fprintf(fp, "%#-16x %s\n", info->addr_cfg.bsbw_addr, "addr_cfg.bsbw_addr");

        fprintf(fp, "%-16d %s\n", info->swreg41.sli_cut, "swreg41.sli_cut");
        fprintf(fp, "%-16d %s\n", info->swreg41.sli_cut_mode, "swreg41.sli_cut_mode");
        fprintf(fp, "%-16d %s\n", info->swreg41.sli_cut_bmod, "swreg41.sli_cut_bmod");
        fprintf(fp, "%-16d %s\n", info->swreg41.sli_max_num, "swreg41.sli_max_num");
        fprintf(fp, "%-16d %s\n", info->swreg41.sli_out_mode, "swreg41.sli_out_mode");
        fprintf(fp, "%-16d %s\n", info->swreg41.sli_cut_cnum, "swreg41.sli_cut_cnum");

        fprintf(fp, "%-16d %s\n", info->swreg42.sli_cut_byte, "swreg42.sli_cut_byte");

        fprintf(fp, "%-16d %s\n", info->swreg43.cime_srch_h, "swreg43.cime_srch_h");
        fprintf(fp, "%-16d %s\n", info->swreg43.cime_srch_v, "swreg43.cime_srch_v");
        fprintf(fp, "%-16d %s\n", info->swreg43.rime_srch_h, "swreg43.rime_srch_h");
        fprintf(fp, "%-16d %s\n", info->swreg43.rime_srch_v, "swreg43.rime_srch_v");

        fprintf(fp, "%-16d %s\n", info->swreg44.pmv_mdst_h, "swreg44.pmv_mdst_h");
        fprintf(fp, "%-16d %s\n", info->swreg44.pmv_mdst_v, "swreg44.pmv_mdst_v");
        fprintf(fp, "%-16d %s\n", info->swreg44.mv_limit, "swreg44.mv_limit");
        fprintf(fp, "%-16d %s\n", info->swreg44.mv_num, "swreg44.mv_num");

        fprintf(fp, "%-16d %s\n", info->swreg45.cach_l1_dtmr, "swreg45.cach_l1_dtmr");

        fprintf(fp, "%-16d %s\n", info->swreg46.rc_en, "swreg46.rc_en");
        fprintf(fp, "%-16d %s\n", info->swreg46.rc_mode, "swreg46.rc_mode");
        fprintf(fp, "%-16d %s\n", info->swreg46.aqmode_en, "swreg46.aqmode_en");
        fprintf(fp, "%-16d %s\n", info->swreg46.aq_strg, "swreg46.aq_strg");
        fprintf(fp, "%-16d %s\n", info->swreg46.rc_ctu_num, "swreg46.rc_ctu_num");

        fprintf(fp, "%-16d %s\n", info->swreg47.bits_error0, "swreg47.bits_error0");
        fprintf(fp, "%-16d %s\n", info->swreg47.bits_error1, "swreg47.bits_error1");
        fprintf(fp, "%-16d %s\n", info->swreg48.bits_error2, "swreg48.bits_error2");
        fprintf(fp, "%-16d %s\n", info->swreg48.bits_error3, "swreg48.bits_error3");
        fprintf(fp, "%-16d %s\n", info->swreg49.bits_error4, "swreg49.bits_error4");
        fprintf(fp, "%-16d %s\n", info->swreg49.bits_error5, "swreg49.bits_error5");
        fprintf(fp, "%-16d %s\n", info->swreg50.bits_error6, "swreg50.bits_error6");
        fprintf(fp, "%-16d %s\n", info->swreg50.bits_error7, "swreg50.bits_error7");
        fprintf(fp, "%-16d %s\n", info->swreg51.bits_error8, "swreg51.bits_error8");

        fprintf(fp, "%-16d %s\n", info->swreg52.qp_adjuest0, "swreg52.qp_adjuest0");
        fprintf(fp, "%-16d %s\n", info->swreg52.qp_adjuest1, "swreg52.qp_adjuest1");
        fprintf(fp, "%-16d %s\n", info->swreg52.qp_adjuest2, "swreg52.qp_adjuest2");
        fprintf(fp, "%-16d %s\n", info->swreg52.qp_adjuest3, "swreg52.qp_adjuest3");
        fprintf(fp, "%-16d %s\n", info->swreg52.qp_adjuest4, "swreg52.qp_adjuest4");
        fprintf(fp, "%-16d %s\n", info->swreg52.qp_adjuest5, "swreg52.qp_adjuest5");
        fprintf(fp, "%-16d %s\n", info->swreg53.qp_adjuest6, "swreg53.qp_adjuest6");
        fprintf(fp, "%-16d %s\n", info->swreg53.qp_adjuest7, "swreg53.qp_adjuest7");
        fprintf(fp, "%-16d %s\n", info->swreg53.qp_adjuest8, "swreg53.qp_adjuest8");

        fprintf(fp, "%-16d %s\n", info->swreg54.rc_qp_mod, "swreg54.rc_qp_mod");
        fprintf(fp, "%-16d %s\n", info->swreg54.rc_fact0, "swreg54.rc_fact0");
        fprintf(fp, "%-16d %s\n", info->swreg54.rc_fact1, "swreg54.rc_fact1");
        fprintf(fp, "%-16d %s\n", info->swreg54.rc_qp_range, "swreg54.rc_qp_range");
        fprintf(fp, "%-16d %s\n", info->swreg54.rc_max_qp, "swreg54.rc_max_qp");
        fprintf(fp, "%-16d %s\n", info->swreg54.rc_min_qp, "swreg54.rc_min_qp");

        fprintf(fp, "%-16d %s\n", info->swreg55.ctu_ebits, "swreg55.ctu_ebits");

        fprintf(fp, "%-16d %s\n", info->swreg56.arb_sel, "swreg56.arb_sel");
        fprintf(fp, "%-16d %s\n", info->swreg56.rdo_mark, "swreg56.rdo_mark");

        fprintf(fp, "%-16d %s\n", info->swreg57.nal_ref_idc, "swreg57.nal_ref_idc");
        fprintf(fp, "%-16d %s\n", info->swreg57.nal_unit_type, "swreg57.nal_unit_type");

        fprintf(fp, "%-16d %s\n", info->swreg58.max_fnum, "swreg58.max_fnum");
        fprintf(fp, "%-16d %s\n", info->swreg58.drct_8x8, "swreg58.drct_8x8");
        fprintf(fp, "%-16d %s\n", info->swreg58.mpoc_lm4, "swreg58.mpoc_lm4");

        fprintf(fp, "%-16d %s\n", info->swreg59.etpy_mode, "swreg59.etpy_mode");
        fprintf(fp, "%-16d %s\n", info->swreg59.trns_8x8, "swreg59.trns_8x8");
        fprintf(fp, "%-16d %s\n", info->swreg59.csip_flg, "swreg59.csip_flg");
        fprintf(fp, "%-16d %s\n", info->swreg59.num_ref0_idx, "swreg59.num_ref0_idx");
        fprintf(fp, "%-16d %s\n", info->swreg59.num_ref1_idx, "swreg59.num_ref1_idx");
        fprintf(fp, "%-16d %s\n", info->swreg59.pic_init_qp, "swreg59.pic_init_qp");
        fprintf(fp, "%-16d %s\n", info->swreg59.cb_ofst, "swreg59.cb_ofst");
        fprintf(fp, "%-16d %s\n", info->swreg59.cr_ofst, "swreg59.cr_ofst");
        fprintf(fp, "%-16d %s\n", info->swreg59.dbf_cp_flg, "swreg59.dbf_cp_flg");

        fprintf(fp, "%-16d %s\n", info->swreg60.sli_type, "swreg60.sli_type");
        fprintf(fp, "%-16d %s\n", info->swreg60.pps_id, "swreg60.pps_id");
        fprintf(fp, "%-16d %s\n", info->swreg60.num_ref_ovrd, "swreg60.num_ref_ovrd");
        fprintf(fp, "%-16d %s\n", info->swreg60.cbc_init_idc, "swreg60.cbc_init_idc");
        fprintf(fp, "%-16d %s\n", info->swreg60.frm_num, "swreg60.frm_num");

        fprintf(fp, "%-16d %s\n", info->swreg61.idr_pid, "swreg61.idr_pid");
        fprintf(fp, "%-16d %s\n", info->swreg61.poc_lsb, "swreg61.poc_lsb");

        fprintf(fp, "%-16d %s\n", info->swreg62.rodr_pic_idx, "swreg62.rodr_pic_idx");
        fprintf(fp, "%-16d %s\n", info->swreg62.ref_list0_rodr, "swreg62.ref_list0_rodr");
        fprintf(fp, "%-16d %s\n", info->swreg62.sli_beta_ofst, "swreg62.sli_beta_ofst");
        fprintf(fp, "%-16d %s\n", info->swreg62.sli_alph_ofst, "swreg62.sli_alph_ofst");
        fprintf(fp, "%-16d %s\n", info->swreg62.dis_dblk_idc, "swreg62.dis_dblk_idc");
        fprintf(fp, "%-16d %s\n", info->swreg62.rodr_pic_num, "swreg62.rodr_pic_num");

        fprintf(fp, "%-16d %s\n", info->swreg63.ltrf_flg, "swreg63.ltrf_flg");
        fprintf(fp, "%-16d %s\n", info->swreg63.arpm_flg, "swreg63.arpm_flg");
        fprintf(fp, "%-16d %s\n", info->swreg63.mmco4_pre, "swreg63.mmco4_pre");
        fprintf(fp, "%-16d %s\n", info->swreg63.mmco_0, "swreg63.mmco_0");
        fprintf(fp, "%-16d %s\n", info->swreg63.dopn_m1_0, "swreg63.dopn_m1_0");

        fprintf(fp, "%-16d %s\n", info->swreg64.mmco_1, "swreg64.mmco_1");
        fprintf(fp, "%-16d %s\n", info->swreg64.dopn_m1_1, "swreg64.dopn_m1_1");

        fprintf(fp, "%-16d %s\n", info->swreg65.osd_en, "swreg65.osd_en");
        fprintf(fp, "%-16d %s\n", info->swreg65.osd_inv, "swreg65.osd_inv");
        fprintf(fp, "%-16d %s\n", info->swreg65.osd_plt_type, "swreg65.osd_plt_type");

        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r0, "swreg66.osd_inv_r0");
        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r1, "swreg66.osd_inv_r1");
        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r2, "swreg66.osd_inv_r2");
        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r3, "swreg66.osd_inv_r3");
        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r4, "swreg66.osd_inv_r4");
        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r5, "swreg66.osd_inv_r5");
        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r6, "swreg66.osd_inv_r6");
        fprintf(fp, "%-16d %s\n", info->swreg66.osd_inv_r7, "swreg66.osd_inv_r7");

        for (k = 0; k < 8; k++) {
            fprintf(fp, "%-16d swreg67_osd_pos[%d].lt_pos_x\n", info->swreg67_osd_pos[k].lt_pos_x, k);
            fprintf(fp, "%-16d swreg67_osd_pos[%d].lt_pos_y\n", info->swreg67_osd_pos[k].lt_pos_y, k);
            fprintf(fp, "%-16d swreg67_osd_pos[%d].rd_pos_x\n", info->swreg67_osd_pos[k].rd_pos_x, k);
            fprintf(fp, "%-16d swreg67_osd_pos[%d].rd_pos_y\n", info->swreg67_osd_pos[k].rd_pos_y, k);
        }
        for (k = 0; k < 8; k++)
            fprintf(fp, "%-16d swreg68_indx_addr_i[%d]\n", info->swreg68_indx_addr_i[k], k);

        for (k = 0; k < 256; k++)
            fprintf(fp, "%#-16x swreg73_osd_indx_tab_i[%d]\n", info->swreg73_osd_indx_tab_i[k], k);

        fprintf(fp, "%#-16x %s\n", info->swreg77.bsbw_addr, "swreg77.bsbw_addr");

        fprintf(fp, "\n");
        fflush(fp);
    } else {
        mpp_log("try to dump data to mpp_syntax_in.txt, but file is not opened");
    }
}
#endif


#if 0
static MPP_RET get_rkv_dbg_info(h264e_hal_rkv_dbg_info *info, h264e_syntax *syn,
                                MppBuffer *hw_in_buf, MppBuffer *hw_output_strm_buf)

{
    RK_S32 k = 0;
    RK_U32 buf_idx = g_frame_read_cnt % RKV_H264E_LINKTABLE_FRAME_NUM;
    h264e_hal_debug_enter();
    mpp_assert(fp_golden_syntax_in);
    memset(syn, 0, sizeof(h264e_syntax));
    memset(info, 0, sizeof(h264e_hal_rkv_dbg_info));

    if (hw_in_buf[buf_idx]) {
        syn->input_luma_addr = mpp_buffer_get_fd(hw_in_buf[buf_idx]);
        syn->input_cb_addr = syn->input_luma_addr; //NOTE: transfer offset in extra_info
        syn->input_cr_addr = syn->input_luma_addr;
    }
    if (hw_output_strm_buf[buf_idx])
        syn->output_strm_addr = mpp_buffer_get_fd(hw_output_strm_buf[buf_idx]);


    if (fp_golden_syntax_in) {
        FILE *fp = fp_golden_syntax_in;
        char temp[512] = {0};
        RK_S32 data = 0;

        if (!fgets(temp, 512, fp))
            return MPP_EOS_STREAM_REACHED;

        H264E_HAL_FSCAN(fp, "%d\n", syn->pic_luma_width);
        H264E_HAL_FSCAN(fp, "%d\n", syn->pic_luma_height);
        H264E_HAL_FSCAN(fp, "%d\n", syn->level_idc);
        H264E_HAL_FSCAN(fp, "%d\n", syn->profile_idc);
        H264E_HAL_FSCAN(fp, "%d\n", syn->frame_coding_type);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg02.lkt_num);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg02.rkvenc_cmd);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg02.enc_cke);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg04.lkt_addr);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.ofe_fnsh);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.lkt_fnsh);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.clr_fnsh);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.ose_fnsh);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.bs_ovflr);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.brsp_ful);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.brsp_err);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.rrsp_err);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg05.tmt_err);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg10.roi_enc);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg10.cur_frm_ref);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg10.mei_stor);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg10.bs_scp);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg10.pic_qp);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg10.slice_int);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg10.node_int);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg11.ppln_enc_lmt);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg11.rfp_load_thrd);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg12.src_bus_edin);


        H264E_HAL_FSCAN(fp, "%d\n", info->swreg13.axi_brsp_cke);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg14.src_aswap);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg14.src_cswap);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg14.src_cfmt);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg14.src_clip_dis);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg15.wght_b2y);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg15.wght_g2y);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg15.wght_r2y);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg16.wght_b2u);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg16.wght_g2u);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg16.wght_r2u);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg17.wght_b2v);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg17.wght_g2v);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg17.wght_r2v);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg18.ofst_rgb2v);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg18.ofst_rgb2u);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg18.ofst_rgb2y);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_tfltr);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_tfltr_we);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_tfltr_bw);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_sfltr);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_mfltr_thrd);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_mfltr_y);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_mfltr_c);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_bfltr_strg);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_bfltr);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_mbflt_odr);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_matf_y);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_matf_c);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_shp_y);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_shp_c);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_shp_div);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_shp_thld);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_mirr);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_rot);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg19.src_matf_itsy);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg20.tfltr_thld_y);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg20.tfltr_thld_c);

        for (k = 0; k < 5; k++)
            H264E_HAL_FSCAN(fp, "%d\n", info->swreg21_scr_stbl[k]);

        for (k = 0; k < 40; k++)
            H264E_HAL_FSCAN(fp, "%d\n", info->swreg22_h3d_tbl[k]);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg23.src_ystrid);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg23.src_cstrid);

        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.adr_srcy);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.adr_srcu);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.adr_srcv);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.ctuc_addr);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.rfpw_addr);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.rfpr_addr);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.dspw_addr);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.dspr_addr);
        H264E_HAL_FSCAN(fp, "%x\n", info->addr_cfg.bsbw_addr);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg41.sli_cut);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg41.sli_cut_mode);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg41.sli_cut_bmod);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg41.sli_max_num);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg41.sli_out_mode);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg41.sli_cut_cnum);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg42.sli_cut_byte);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg43.cime_srch_h);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg43.cime_srch_v);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg43.rime_srch_h);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg43.rime_srch_v);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg44.pmv_mdst_h);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg44.pmv_mdst_v);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg44.mv_limit);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg44.mv_num);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg45.cach_l1_dtmr);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg46.rc_en);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg46.rc_mode);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg46.aqmode_en);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg46.aq_strg);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg46.rc_ctu_num);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg47.bits_error0);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg47.bits_error1);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg48.bits_error2);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg48.bits_error3);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg49.bits_error4);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg49.bits_error5);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg50.bits_error6);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg50.bits_error7);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg51.bits_error8);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg52.qp_adjuest0);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg52.qp_adjuest1);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg52.qp_adjuest2);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg52.qp_adjuest3);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg52.qp_adjuest4);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg52.qp_adjuest5);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg53.qp_adjuest6);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg53.qp_adjuest7);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg53.qp_adjuest8);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg54.rc_qp_mod);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg54.rc_fact0);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg54.rc_fact1);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg54.rc_qp_range);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg54.rc_max_qp);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg54.rc_min_qp);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg55.ctu_ebits);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg56.arb_sel);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg56.rdo_mark);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg57.nal_ref_idc);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg57.nal_unit_type);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg58.max_fnum);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg58.drct_8x8);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg58.mpoc_lm4);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.etpy_mode);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.trns_8x8);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.csip_flg);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.num_ref0_idx);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.num_ref1_idx);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.pic_init_qp);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.cb_ofst);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.cr_ofst);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg59.dbf_cp_flg);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg60.sli_type);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg60.pps_id);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg60.num_ref_ovrd);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg60.cbc_init_idc);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg60.frm_num);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg61.idr_pid);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg61.poc_lsb);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg62.rodr_pic_idx);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg62.ref_list0_rodr);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg62.sli_beta_ofst);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg62.sli_alph_ofst);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg62.dis_dblk_idc);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg62.rodr_pic_num);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg63.ltrf_flg);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg63.arpm_flg);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg63.mmco4_pre);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg63.mmco_0);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg63.dopn_m1_0);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg64.mmco_1);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg64.dopn_m1_1);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg65.osd_en);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg65.osd_inv);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg65.osd_plt_type);

        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r0);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r1);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r2);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r3);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r4);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r5);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r6);
        H264E_HAL_FSCAN(fp, "%d\n", info->swreg66.osd_inv_r7);

        for (k = 0; k < 8; k++) {
            H264E_HAL_FSCAN(fp, "%d\n", info->swreg67_osd_pos[k].lt_pos_x);
            H264E_HAL_FSCAN(fp, "%d\n", info->swreg67_osd_pos[k].lt_pos_y);
            H264E_HAL_FSCAN(fp, "%d\n", info->swreg67_osd_pos[k].rd_pos_x);
            H264E_HAL_FSCAN(fp, "%d\n", info->swreg67_osd_pos[k].rd_pos_y);
        }
        for (k = 0; k < 8; k++)
            H264E_HAL_FSCAN(fp, "%d\n", info->swreg68_indx_addr_i[k]);

        for (k = 0; k < 256; k++)
            H264E_HAL_FSCAN(fp, "%x\n", info->swreg73_osd_indx_tab_i[k]);

        H264E_HAL_FSCAN(fp, "%x\n", info->swreg77.bsbw_addr);

        H264E_HAL_FSCAN(fp, "%x\n", syn->keyframe_max_interval);

        fgets(temp, 512, fp);

        //set values actually used
        syn->slice_type = info->swreg60.sli_type;
        syn->pps_id = info->swreg60.pps_id;
        syn->cabac_init_idc = info->swreg60.cbc_init_idc;
        syn->pic_order_cnt_lsb = info->swreg61.poc_lsb;
        syn->idr_pic_id = info->swreg61.idr_pid;
        syn->pic_init_qp = info->swreg59.pic_init_qp;
        syn->qp = info->swreg10.pic_qp;
        syn->frame_num = info->swreg60.frm_num;
        syn->input_image_format = info->swreg14.src_cfmt;
        syn->transform8x8_mode = info->swreg59.trns_8x8;
    }

    h264e_hal_debug_leave();
    return MPP_OK;
}
#endif


static MPP_RET get_rkv_syntax_in( h264e_syntax *syn, MppBuffer *hw_in_buf, MppBuffer *hw_output_strm_buf, h264e_hal_test_cfg *cfg)

{
    h264e_hal_rkv_csp_info csp_info;
    RK_U32 buf_idx = g_frame_read_cnt % RKV_H264E_LINKTABLE_FRAME_NUM;
    h264e_hal_debug_enter();
    //mpp_assert(fp_golden_syntax_in);
    memset(syn, 0, sizeof(h264e_syntax));

    if (hw_in_buf[buf_idx]) {
        syn->input_luma_addr = mpp_buffer_get_fd(hw_in_buf[buf_idx]);
        syn->input_cb_addr = syn->input_luma_addr; //NOTE: transfer offset in extra_info
        syn->input_cr_addr = syn->input_luma_addr;
    }
    if (hw_output_strm_buf[buf_idx])
        syn->output_strm_addr = mpp_buffer_get_fd(hw_output_strm_buf[buf_idx]);

#if RKV_H264E_SDK_TEST
    mpp_log("make syntax begin");
    syn->pic_luma_width = cfg->pic_width;
    syn->pic_luma_height = cfg->pic_height;
    syn->level_idc = H264_LEVEL_4_1;
    syn->profile_idc = H264_PROFILE_HIGH;
    mpp_log("syn->level_idc %d", syn->level_idc);
    mpp_log("syn->profile_idc %d", syn->profile_idc);
    syn->keyframe_max_interval = 30;
    if (g_frame_cnt == 0 || g_frame_cnt % syn->keyframe_max_interval == 0) {
        syn->frame_coding_type = 1; //intra
        syn->slice_type = 2;
    } else {
        syn->frame_coding_type = 0; //inter
        syn->slice_type = 0;
    }
    syn->qp = 26;
    csp_info.fmt = H264E_RKV_CSP_YUV420P;
    csp_info.cswap = 0; //TODO:
    csp_info.aswap = 0; //TODO:
    syn->input_image_format = h264e_rkv_revert_csp(csp_info);

    syn->enable_cabac = 1;
    syn->pic_init_qp = 26;
    syn->chroma_qp_index_offset = 0;
    syn->second_chroma_qp_index_offset = 0;

    syn->pps_id = 0 ;
    syn->frame_num = 0;
    syn->cabac_init_idc = 0;


    syn->idr_pic_id = 0;
    syn->pic_order_cnt_lsb = 0;


    mpp_log("make syntax end");
#else
    if (fp_golden_syntax_in) {
        FILE *fp = fp_golden_syntax_in;
        char temp[512] = {0};
        RK_S32 data = 0;

        if (!fgets(temp, 512, fp))
            return MPP_EOS_STREAM_REACHED;

        H264E_HAL_FSCAN(fp, "%d\n", syn->pic_luma_width);
        H264E_HAL_FSCAN(fp, "%d\n", syn->pic_luma_height);
        H264E_HAL_FSCAN(fp, "%d\n", syn->level_idc);
        H264E_HAL_FSCAN(fp, "%d\n", syn->profile_idc);
        H264E_HAL_FSCAN(fp, "%d\n", syn->frame_coding_type);
        H264E_HAL_FSCAN(fp, "%d\n", syn->qp);
        H264E_HAL_FSCAN(fp, "%d\n", syn->input_image_format);

        H264E_HAL_FSCAN(fp, "%d\n", syn->enable_cabac);
        H264E_HAL_FSCAN(fp, "%d\n", syn->pic_init_qp);
        H264E_HAL_FSCAN(fp, "%d\n", syn->chroma_qp_index_offset);
        H264E_HAL_FSCAN(fp, "%d\n", syn->second_chroma_qp_index_offset);

        H264E_HAL_FSCAN(fp, "%d\n", syn->slice_type);
        H264E_HAL_FSCAN(fp, "%d\n", syn->pps_id);
        H264E_HAL_FSCAN(fp, "%d\n", syn->frame_num);
        H264E_HAL_FSCAN(fp, "%d\n", syn->cabac_init_idc);


        H264E_HAL_FSCAN(fp, "%d\n", syn->idr_pic_id);
        H264E_HAL_FSCAN(fp, "%d\n", syn->pic_order_cnt_lsb);

        H264E_HAL_FSCAN(fp, "%x\n", syn->keyframe_max_interval);

        fgets(temp, 512, fp);

        /* adjust */
        csp_info.fmt = syn->input_image_format;
        csp_info.cswap = 0; //TODO:
        csp_info.aswap = 0; //TODO:
        syn->input_image_format = h264e_rkv_revert_csp(csp_info);
    } else {
        mpp_err("rkv_syntax_in.txt doesn't exits");
    }
#endif
    syn->output_strm_limit_size = (RK_U32)(syn->pic_luma_width * syn->pic_luma_height * 3 / 2);

    h264e_hal_debug_leave();
    return MPP_OK;
}

static MPP_RET h264e_hal_test_parse_options(int arg_num, char **arg_str, h264e_hal_test_cfg *cfg)
{
    RK_S32 k = 0;
    memset(cfg, 0, sizeof(h264e_hal_test_cfg));

    cfg->num_frames = 30;
    for (k = 1; k < arg_num; k++) {
        if (!strcmp("-hw", arg_str[k])) {
            cfg->hw_mode = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("-yuv", arg_str[k])) {
            strcpy(cfg->input_yuv_file_path, arg_str[k + 1]);
            k++;
        }
        if (!strcmp("-syn", arg_str[k])) {
            strcpy(cfg->input_syntax_file_path, arg_str[k + 1]);
            k++;
        }

        if (!strcmp("-w", arg_str[k])) {
            cfg->pic_width = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("-h", arg_str[k])) {
            cfg->pic_height = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("-fmt", arg_str[k])) {
            cfg->src_format = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("-frame", arg_str[k])) {
            cfg->num_frames = atoi(arg_str[k + 1]);
            k++;
        }

        /* coverage test */
        if (!strcmp("--test-qp", arg_str[k])) {
            cfg->test.qp = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("--test-preproc", arg_str[k])) {
            cfg->test.preproc = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("--test-osd", arg_str[k])) {
            cfg->test.osd = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("--test-mbrc", arg_str[k])) {
            cfg->test.mbrc = atoi(arg_str[k + 1]);
            k++;
        }
        if (!strcmp("--test-roi", arg_str[k])) {
            cfg->test.roi = atoi(arg_str[k + 1]);
            k++;
        }
    }


    if (!cfg->input_yuv_file_path) {
        mpp_log("test param parse error: input_yuv_file_path is NULL");
        return MPP_NOK;
    }
#if RKV_H264E_SDK_TEST
    if (!cfg->pic_width) {
        mpp_log("test param parse error: pic_width is 0");
        return MPP_NOK;
    }
    if (!cfg->pic_height) {
        mpp_log("test param parse error: pic_height is 0");
        return MPP_NOK;
    }
#else
    if (!cfg->input_syntax_file_path) {
        mpp_log("test param parse error: input_syntax_file_path is NULL");
        return MPP_NOK;
    }
#endif

    if (cfg->hw_mode == 0)
        test_device_id = HAL_RKVENC;
    else if (cfg->hw_mode == 1)
        test_device_id = HAL_VEPU;
    else {
        mpp_log("test param parse error: hw_mode is %d", cfg->hw_mode);
        return MPP_NOK;
    }

    mpp_log("======== hal converage test cfg (st) =======");
    if (cfg->test.qp)
        mpp_log("cfg->test.qp %d", cfg->test.qp);
    if (cfg->test.preproc)
        mpp_log("cfg->test.preproc %d", cfg->test.preproc);
    if (cfg->test.osd)
        mpp_log("cfg->test.osd %d", cfg->test.osd);
    if (cfg->test.mbrc)
        mpp_log("cfg->test.mbrc %d", cfg->test.mbrc);
    if (cfg->test.roi)
        mpp_log("cfg->test.roi %d", cfg->test.roi);
    mpp_log("======== hal converage test cfg (ed) =======");

    return MPP_OK;
}

static void h264e_hal_test_init(h264e_hal_context *ctx, HalTaskInfo *task_info)
{
    memset(ctx, 0, sizeof(h264e_hal_context));
    memset(task_info, 0, sizeof(HalTaskInfo));
}

static void h264e_hal_test_deinit(h264e_hal_context *ctx, HalTaskInfo *task_info)
{
    (void)ctx;
    (void)task_info;
}

MPP_RET h264_hal_test_call_back(void *control, void *feedback)
{
    (void)control;
    h264e_feedback *fb = (h264e_feedback *)feedback;
    (void)fb;

    mpp_log("h264_hal_test_call_back, enter");
    mpp_log("h264_hal_test_call_back, leave");

    return MPP_OK;
}

static void h264e_hal_set_extra_info_cfg(h264e_control_extra_info_cfg *info, h264e_syntax *syn)
{
    info->chroma_qp_index_offset        = syn->chroma_qp_index_offset;
    info->enable_cabac                  = syn->enable_cabac;
    info->pic_init_qp                   = syn->pic_init_qp;
    info->pic_luma_height               = syn->pic_luma_height;
    info->pic_luma_width                = syn->pic_luma_width;
    info->transform8x8_mode             = syn->transform8x8_mode;

    info->input_image_format            = syn->input_image_format;
    info->profile_idc                   = syn->profile_idc;
    info->level_idc                     = syn->level_idc;
    info->keyframe_max_interval         = syn->keyframe_max_interval;
    info->second_chroma_qp_index_offset = syn->second_chroma_qp_index_offset;
    info->pps_id                        = syn->pps_id;

    info->frame_rate                    = 30;
}

MPP_RET h264e_hal_vpu_test()
{
    MPP_RET ret = MPP_OK;
    h264e_hal_context ctx;
    MppHalCfg hal_cfg;
    HalTaskInfo task_info;
    h264e_syntax syntax_data;
    h264e_control_extra_info_cfg extra_info_cfg;
    MppPacket extra_info_pkt;
    RK_U8 extra_info_buf[H264E_MAX_PACKETED_PARAM_SIZE] = {0};
    MppBufferGroup hw_input_buf_grp = NULL;
    MppBufferGroup hw_output_buf_grp = NULL;
    MppBuffer hw_input_buf = NULL; //Y, U, V
    MppBuffer hw_output_strm_buf = NULL;
    RK_U32 frame_luma_size = 0;
    RK_S64 t0;

    RK_U8 *input_sw_buf = mpp_malloc(RK_U8, MAX_FRAME_TOTAL_SIZE);

    mpp_packet_init(&extra_info_pkt, (void *)extra_info_buf, H264E_MAX_PACKETED_PARAM_SIZE);

    get_vpu_syntax_in(&syntax_data, hw_input_buf, hw_output_strm_buf, frame_luma_size);
    if (fp_golden_syntax_in)
        fseek(fp_golden_syntax_in, 0L, SEEK_SET);

    frame_luma_size = syntax_data.pic_luma_width * syntax_data.pic_luma_height;

    h264e_hal_test_init(&ctx, &task_info);

    if (MPP_OK != mpp_buffer_group_get_internal(&hw_input_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_input_buf_grp get failed, test is ended early");
        goto __test_end;
    }
    if (MPP_OK != mpp_buffer_group_get_internal(&hw_output_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_output_buf_grp get failed, test is ended early");
        goto __test_end;
    }

    mpp_buffer_get(hw_input_buf_grp, &hw_input_buf, frame_luma_size * 3 / 2);
    mpp_buffer_get(hw_output_buf_grp, &hw_output_strm_buf, 1024 * 1024 * 2);

    task_info.enc.syntax.data = (void *)&syntax_data;
    task_info.enc.input = hw_input_buf;
    task_info.enc.output = hw_input_buf;

    hal_cfg.hal_int_cb.callBack = h264_hal_test_call_back;
    hal_cfg.hal_int_cb.opaque = NULL; //control context
    hal_cfg.device_id = test_device_id;
    hal_h264e_init(&ctx, &hal_cfg);

    h264e_hal_set_extra_info_cfg(&extra_info_cfg, &syntax_data);
    hal_h264e_vpu_control(&ctx, MPP_ENC_SET_EXTRA_INFO, &extra_info_cfg);
    hal_h264e_vpu_control(&ctx, MPP_ENC_GET_EXTRA_INFO, &extra_info_pkt);

    do {
        /* get golden input */
        if (MPP_EOS_STREAM_REACHED == get_vpu_syntax_in(&syntax_data, hw_input_buf, hw_output_strm_buf, frame_luma_size)) {
            mpp_log("syntax input file end, total %d frames are encoded, test is ended", g_frame_cnt);
            break;
        }

        ret = get_h264e_yuv_in_one_frame(input_sw_buf, &syntax_data, hw_input_buf);

        if (ret == MPP_EOS_STREAM_REACHED) {
            mpp_log("yuv file end, total %d frames are encoded, test is ended", g_frame_cnt);
            break;
        } else if (ret == MPP_NOK) {
            mpp_err("read yuv file failed, test is ended early");
            goto __test_end;
        }


        /* generate registers */
        hal_h264e_gen_regs(&ctx, &task_info);


        /* run hardware */
        t0 = mpp_time();
        mpp_log("hal_h264e_start time : %lld ", (RK_S64)(t0 / 1000));

        hal_h264e_start(&ctx, &task_info);
        hal_h264e_wait(&ctx, &task_info);

        g_frame_cnt ++;
    } while (1);


__test_end:
    hal_h264e_deinit(&ctx);
    h264e_hal_test_deinit(&ctx, &task_info);

    mpp_packet_deinit(&extra_info_pkt);
    //free sw buf
    if (input_sw_buf) {
        mpp_free(input_sw_buf);
        input_sw_buf = NULL;
    }

    //free hw buf
    if (hw_input_buf)
        mpp_buffer_put(hw_input_buf);
    if (hw_output_strm_buf)
        mpp_buffer_put(hw_output_strm_buf);

    if (hw_input_buf_grp)
        mpp_buffer_group_put(hw_input_buf_grp);
    if (hw_output_buf_grp)
        mpp_buffer_group_put(hw_output_buf_grp);

    return MPP_OK;
}


MPP_RET h264e_hal_rkv_test(h264e_hal_test_cfg *test_cfg)
{
    RK_S32 k = 0;
    MPP_RET ret = MPP_OK;
    h264e_hal_context ctx;
    MppHalCfg hal_cfg;
    HalTaskInfo task_info;
    h264e_control_extra_info_cfg extra_info_cfg;
    MppPacket extra_info_pkt;
    RK_U8 extra_info_buf[H264E_MAX_PACKETED_PARAM_SIZE] = {0};
    //h264e_hal_rkv_dbg_info dbg_info;
    h264e_syntax syntax_data[RKV_H264E_LINKTABLE_FRAME_NUM];
    MppBufferGroup hw_input_buf_grp = NULL;
    MppBufferGroup hw_output_buf_grp = NULL;
    MppBuffer hw_input_buf_mul[RKV_H264E_LINKTABLE_FRAME_NUM] = {NULL};
    MppBuffer hw_output_strm_buf_mul[RKV_H264E_LINKTABLE_FRAME_NUM] = {NULL};
    RK_U32 frame_luma_stride = 0;
    RK_S64 t0;

    mpp_packet_init(&extra_info_pkt, (void *)extra_info_buf, H264E_MAX_PACKETED_PARAM_SIZE);


    get_rkv_syntax_in(&syntax_data[0], hw_input_buf_mul, hw_output_strm_buf_mul, test_cfg);
    if (fp_golden_syntax_in)
        fseek(fp_golden_syntax_in, 0L, SEEK_SET);


    frame_luma_stride = ((syntax_data[0].pic_luma_width + 15) & (~15)) * ((syntax_data[0].pic_luma_height + 15) & (~15));

    h264e_hal_test_init(&ctx, &task_info);


    if (MPP_OK != mpp_buffer_group_get_internal(&hw_input_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_input_buf_grp get failed, test is ended early");
        goto __test_end;
    }
    if (MPP_OK != mpp_buffer_group_get_internal(&hw_output_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_output_buf_grp get failed, test is ended early");
        goto __test_end;
    }

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++)
        mpp_buffer_get(hw_input_buf_grp, &hw_input_buf_mul[k], frame_luma_stride * 3 / 2);

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++)
        mpp_buffer_get(hw_output_buf_grp, &hw_output_strm_buf_mul[k], syntax_data[0].pic_luma_width * syntax_data[0].pic_luma_height * 3 / 2);


    hal_cfg.hal_int_cb.callBack = h264_hal_test_call_back;
    hal_cfg.hal_int_cb.opaque = NULL; //control context
    hal_cfg.device_id = test_device_id;
    hal_h264e_init(&ctx, &hal_cfg);

    ctx.test_cfg = (void *)&test_cfg->test;

    h264e_hal_set_extra_info_cfg(&extra_info_cfg, &syntax_data[0]); //TODO: use dbg info for input instead
    hal_h264e_rkv_control(&ctx, MPP_ENC_SET_EXTRA_INFO, &extra_info_cfg);
    hal_h264e_rkv_control(&ctx, MPP_ENC_GET_EXTRA_INFO, &extra_info_pkt);

    do {
        /* get golden input */
        if (g_frame_read_cnt <= g_frame_cnt) {
            h264e_syntax *syn = NULL;
            RK_S32 frame_num = RKV_H264E_ENC_MODE == 1 ? 1 : RKV_H264E_LINKTABLE_FRAME_NUM;
            mpp_log("read %d frames input", frame_num);
            for (k = 0; k < frame_num; k++, g_frame_read_cnt++) {
                syn = &syntax_data[g_frame_read_cnt % RKV_H264E_LINKTABLE_FRAME_NUM];
                if (MPP_EOS_STREAM_REACHED == get_rkv_syntax_in(syn, hw_input_buf_mul, hw_output_strm_buf_mul, test_cfg)) {
                    mpp_log("syntax input file end, total %d frames are encoded, test is ended", g_frame_cnt);
                    goto __test_end;
                }
                ret = get_rkv_h264e_yuv_in_frame(syn, hw_input_buf_mul);
                if (ret == MPP_EOS_STREAM_REACHED) {
                    if (g_frame_cnt == test_cfg->num_frames) {
                        mpp_log("total %d frames are encoded, test is ended", g_frame_cnt);
                        goto __test_end;
                    } else {
                        fseek(fp_h264e_yuv_in, 0L, SEEK_SET);
                        ret = get_rkv_h264e_yuv_in_frame(syn, hw_input_buf_mul);
                    }
                } else if (ret == MPP_NOK) {
                    mpp_err("read yuv file failed, test is ended early");
                    goto __test_end;
                }
            }
        }

        task_info.enc.syntax.data = (void *)&syntax_data[g_frame_cnt % RKV_H264E_LINKTABLE_FRAME_NUM];
        task_info.enc.input = hw_input_buf_mul[g_frame_cnt % RKV_H264E_LINKTABLE_FRAME_NUM];
        task_info.enc.output = hw_output_strm_buf_mul[g_frame_cnt % RKV_H264E_LINKTABLE_FRAME_NUM];
        /* generate registers */
        hal_h264e_gen_regs(&ctx, &task_info);


        /* run hardware */
        t0 = mpp_time();
        mpp_log("hal_h264e_start time : %lld ", (RK_S64)(t0 / 1000));

        hal_h264e_start(&ctx, &task_info);
        hal_h264e_wait(&ctx, &task_info);

        g_frame_cnt ++;

        if (g_frame_cnt == test_cfg->num_frames) {
            mpp_log("test_cfg->num_frames %d reached, end test", test_cfg->num_frames);
            goto __test_end;
        }

    } while (1);


__test_end:
    mpp_packet_deinit(&extra_info_pkt);

    hal_h264e_deinit(&ctx);
    h264e_hal_test_deinit(&ctx, &task_info);

    //free hw buf
    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (hw_input_buf_mul[k])
            mpp_buffer_put(hw_input_buf_mul[k]);
        if (hw_output_strm_buf_mul[k])
            mpp_buffer_put(hw_output_strm_buf_mul[k]);
    }

    if (hw_input_buf_grp)
        mpp_buffer_group_put(hw_input_buf_grp);
    if (hw_output_buf_grp)
        mpp_buffer_group_put(hw_output_buf_grp);

    return MPP_OK;
}


int main(int argc, char **argv)
{
    h264e_hal_test_cfg test_cfg;

    mpp_log("******* h264e hal test start *******");

    if (MPP_OK != h264e_hal_test_parse_options(argc, argv, &test_cfg)) {
        mpp_err("parse opitons failed, test is ended early");
        goto __test_end;
    }

    if (test_device_id == HAL_VEPU) {
        if (MPP_OK != h264e_vpu_test_open_files(test_cfg)) {
            mpp_err("open files error, test is ended early");
            goto __test_end;
        }
        mpp_log("choose h264e_hal_vpu_test");
        h264e_hal_vpu_test();
    } else {
        if (MPP_OK != h264e_rkv_test_open_files(test_cfg)) {
            mpp_err("open files error, test is ended early");
            goto __test_end;
        }
        mpp_log("choose h264e_hal_rkv_test");
        h264e_hal_rkv_test(&test_cfg);
    }

__test_end:

    h264e_test_close_files();

    mpp_log("******* h264e hal test end *******");
    return 0;
}
