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
#include "hal_h264e_com.h"
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

static MPP_RET h264e_rkv_test_gen_osd_data(MppEncOSDData *osd_data, MppBuffer *hw_osd_buf_mul)
{
    RK_U32 k = 0, buf_size = 0;
    RK_U8 data = 0;

    osd_data->num_region = 8;
    osd_data->buf = hw_osd_buf_mul[g_frame_read_cnt % RKV_H264E_LINKTABLE_FRAME_NUM];
    for (k = 0; k < osd_data->num_region; k++) {
        osd_data->region[k].enable = 1;
        osd_data->region[k].inverse = 0;
        osd_data->region[k].start_mb_x = k * 4;
        osd_data->region[k].start_mb_y = k * 4;
        osd_data->region[k].num_mb_x = 3;
        osd_data->region[k].num_mb_y = 3;

        buf_size = osd_data->region[k].num_mb_x * osd_data->region[k].num_mb_y * 256;
        osd_data->region[k].buf_offset = k * buf_size;

        data = k;
        memset((RK_U8 *)mpp_buffer_get_ptr(osd_data->buf) + osd_data->region[k].buf_offset, data, buf_size);
    }

    return MPP_OK;
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

static MPP_RET get_rkv_h264e_yuv_in_frame(H264eHwCfg *syn, MppBuffer *hw_buf)
{
    RK_U32 read_ret = 0;
    RK_U32 frame_luma_size = syn->width * syn->height;
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

static MPP_RET get_h264e_yuv_in_one_frame(RK_U8 *sw_buf, H264eHwCfg *syn, MppBuffer hw_buf)
{
    RK_U32 read_ret = 0;
    RK_U32 frame_luma_size = syn->width * syn->height;
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

static MPP_RET get_vpu_syntax_in(H264eHwCfg *syn, MppBuffer hw_in_buf, MppBuffer hw_output_strm_buf, RK_U32 frame_luma_size)
{
    RK_S32 k = 0;
    mpp_assert(fp_golden_syntax_in);
    memset(syn, 0, sizeof(H264eHwCfg));

    if (fp_golden_syntax_in) {
        char temp[512] = {0};
        RK_S32 data = 0;
        if (!fgets(temp, 512, fp_golden_syntax_in))
            return MPP_EOS_STREAM_REACHED;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->frame_type = data;


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
        syn->inter4x4_disabled = data;

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
        syn->width = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->height = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->input_format = data;

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
    syn->input_format = h264e_vpu_revert_csp(syn->input_format);

    return MPP_OK;
}

static MPP_RET get_rkv_syntax_in( H264eHwCfg *syn, MppBuffer *hw_in_buf, MppBuffer *hw_output_strm_buf, h264e_hal_test_cfg *cfg)

{
    h264e_hal_rkv_csp_info csp_info;
    RK_U32 buf_idx = g_frame_read_cnt % RKV_H264E_LINKTABLE_FRAME_NUM;
    h264e_hal_debug_enter();
    //mpp_assert(fp_golden_syntax_in);
    memset(syn, 0, sizeof(H264eHwCfg));

    if (hw_in_buf[buf_idx]) {
        syn->input_luma_addr = mpp_buffer_get_fd(hw_in_buf[buf_idx]);
        syn->input_cb_addr = syn->input_luma_addr; //NOTE: transfer offset in extra_info
        syn->input_cr_addr = syn->input_luma_addr;
    }
    if (hw_output_strm_buf[buf_idx])
        syn->output_strm_addr = mpp_buffer_get_fd(hw_output_strm_buf[buf_idx]);

#if RKV_H264E_SDK_TEST
    mpp_log("make syntax begin");
    syn->width = cfg->pic_width;
    syn->height = cfg->pic_height;
    syn->level_idc = H264_LEVEL_4_1;
    syn->profile_idc = H264_PROFILE_HIGH;
    mpp_log("syn->level_idc %d", syn->level_idc);
    mpp_log("syn->profile_idc %d", syn->profile_idc);
    syn->keyframe_max_interval = 30;
    if (g_frame_cnt == 0 || g_frame_cnt % syn->keyframe_max_interval == 0) {
        syn->frame_type = 1; //intra
    } else {
        syn->frame_type = 0; //inter
    }
    syn->qp = 26;
    csp_info.fmt = H264E_RKV_CSP_YUV420P;
    csp_info.cswap = 0; //TODO:
    csp_info.aswap = 0; //TODO:
    syn->input_format = h264e_rkv_revert_csp(csp_info);

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

        H264E_HAL_FSCAN(fp, "%d\n", syn->width);
        H264E_HAL_FSCAN(fp, "%d\n", syn->height);
        H264E_HAL_FSCAN(fp, "%d\n", syn->level_idc);
        H264E_HAL_FSCAN(fp, "%d\n", syn->profile_idc);
        H264E_HAL_FSCAN(fp, "%d\n", syn->frame_type);
        H264E_HAL_FSCAN(fp, "%d\n", syn->qp);
        H264E_HAL_FSCAN(fp, "%d\n", syn->input_format);

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
        csp_info.fmt = syn->input_format;
        csp_info.cswap = 0; //TODO:
        csp_info.aswap = 0; //TODO:
        syn->input_format = h264e_rkv_revert_csp(csp_info);
    } else {
        mpp_err("rkv_syntax_in.txt doesn't exits");
    }
#endif
    syn->output_strm_limit_size = (RK_U32)(syn->width * syn->height * 3 / 2);

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

static void h264e_hal_set_extra_info_cfg(h264e_control_extra_info_cfg *info, H264eHwCfg *syn)
{
    info->chroma_qp_index_offset        = syn->chroma_qp_index_offset;
    info->enable_cabac                  = syn->enable_cabac;
    info->pic_init_qp                   = syn->pic_init_qp;
    info->pic_luma_height               = syn->height;
    info->pic_luma_width                = syn->width;
    info->transform8x8_mode             = syn->transform8x8_mode;

    info->input_image_format            = syn->input_format;
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
    H264eHwCfg syntax_data;
    h264e_control_extra_info_cfg extra_info_cfg;
    MppPacket extra_info_pkt;
    RK_U8 extra_info_buf[H264E_EXTRA_INFO_BUF_SIZE] = {0};
    MppBufferGroup hw_input_buf_grp = NULL;
    MppBufferGroup hw_output_buf_grp = NULL;
    MppBuffer hw_input_buf = NULL; //Y, U, V
    MppBuffer hw_output_strm_buf = NULL;
    RK_U32 frame_luma_size = 0;
    RK_S64 t0;

    RK_U8 *input_sw_buf = mpp_malloc(RK_U8, MAX_FRAME_TOTAL_SIZE);

    mpp_packet_init(&extra_info_pkt, (void *)extra_info_buf, H264E_EXTRA_INFO_BUF_SIZE);

    get_vpu_syntax_in(&syntax_data, hw_input_buf, hw_output_strm_buf, frame_luma_size);
    if (fp_golden_syntax_in)
        fseek(fp_golden_syntax_in, 0L, SEEK_SET);

    frame_luma_size = syntax_data.width * syntax_data.height;

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
    RK_U8 extra_info_buf[H264E_EXTRA_INFO_BUF_SIZE] = {0};
    H264eHwCfg syntax_data[RKV_H264E_LINKTABLE_FRAME_NUM];
    MppBufferGroup hw_input_buf_grp = NULL;
    MppBufferGroup hw_output_buf_grp = NULL;
    MppBuffer hw_input_buf_mul[RKV_H264E_LINKTABLE_FRAME_NUM] = {NULL};
    MppBuffer hw_output_strm_buf_mul[RKV_H264E_LINKTABLE_FRAME_NUM] = {NULL};
    MppBuffer hw_osd_buf_mul[RKV_H264E_LINKTABLE_FRAME_NUM] = {NULL};
    RK_U32 frame_luma_stride = 0;
    MppEncOSDPlt osd_plt;
    MppEncOSDData osd_data;
    RK_S64 t0;
    RK_U32 plt_table[8] = {
        MPP_ENC_OSD_PLT_WHITE,
        MPP_ENC_OSD_PLT_YELLOW,
        MPP_ENC_OSD_PLT_CYAN,
        MPP_ENC_OSD_PLT_GREEN,
        MPP_ENC_OSD_PLT_TRANS,
        MPP_ENC_OSD_PLT_RED,
        MPP_ENC_OSD_PLT_BLUE,
        MPP_ENC_OSD_PLT_BLACK
    };

    mpp_packet_init(&extra_info_pkt, (void *)extra_info_buf, H264E_EXTRA_INFO_BUF_SIZE);


    get_rkv_syntax_in(&syntax_data[0], hw_input_buf_mul, hw_output_strm_buf_mul, test_cfg);
    if (fp_golden_syntax_in)
        fseek(fp_golden_syntax_in, 0L, SEEK_SET);


    frame_luma_stride = ((syntax_data[0].width + 15) & (~15)) * ((syntax_data[0].height + 15) & (~15));

    h264e_hal_test_init(&ctx, &task_info);


    if (MPP_OK != mpp_buffer_group_get_internal(&hw_input_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_input_buf_grp get failed, test is ended early");
        goto __test_end;
    }
    if (MPP_OK != mpp_buffer_group_get_internal(&hw_output_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_output_buf_grp get failed, test is ended early");
        goto __test_end;
    }

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        mpp_buffer_get(hw_input_buf_grp, &hw_input_buf_mul[k], frame_luma_stride * 3 / 2);
        mpp_buffer_get(hw_input_buf_grp, &hw_osd_buf_mul[k], frame_luma_stride);
    }

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++)
        mpp_buffer_get(hw_output_buf_grp, &hw_output_strm_buf_mul[k], syntax_data[0].width * syntax_data[0].height * 3 / 2);


    hal_cfg.hal_int_cb.callBack = h264_hal_test_call_back;
    hal_cfg.hal_int_cb.opaque = NULL; //control context
    hal_cfg.device_id = test_device_id;
    hal_h264e_init(&ctx, &hal_cfg);

    ctx.test_cfg = (void *)&test_cfg->test;

    h264e_hal_set_extra_info_cfg(&extra_info_cfg, &syntax_data[0]); //TODO: use dbg info for input instead
    hal_h264e_rkv_control(&ctx, MPP_ENC_SET_EXTRA_INFO, &extra_info_cfg);
    hal_h264e_rkv_control(&ctx, MPP_ENC_GET_EXTRA_INFO, &extra_info_pkt);

    if (osd_plt.buf) {
        for (k = 0; k < 256; k++) {
            osd_plt.buf[k] = plt_table[k % 8];
        }
    }

    hal_h264e_rkv_control(&ctx, MPP_ENC_SET_OSD_PLT_CFG, (void *)&osd_plt);

    do {
        /* get golden input */
        if (g_frame_read_cnt <= g_frame_cnt) {
            H264eHwCfg *syn = NULL;
            RK_S32 frame_num = RKV_H264E_ENC_MODE == 1 ? 1 : RKV_H264E_LINKTABLE_FRAME_NUM;
            mpp_log("read %d frames input", frame_num);
            for (k = 0; k < frame_num; k++, g_frame_read_cnt++) {

                h264e_rkv_test_gen_osd_data(&osd_data, hw_osd_buf_mul);
                hal_h264e_rkv_control(&ctx, MPP_ENC_SET_OSD_DATA_CFG, (void *)&osd_data);

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
        if (hw_osd_buf_mul[k])
            mpp_buffer_put(hw_osd_buf_mul[k]);
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
