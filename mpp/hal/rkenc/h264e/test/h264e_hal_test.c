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
#include <sys/time.h>
#include "mpp_common.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "hal_h264e_api.h"
#include "hal_h264e.h"
#include "hal_h264e_vpu.h"
#include "hal_h264e_rkv.h"

#define H264E_HAL_FPGA_TEST             0
#define H264E_HAL_FPGA_FAST_MODE        0

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
extern const RK_S32 h264_context_init_intra[460][2];
extern const RK_S32 h264_context_init[3][460][2];
extern const RK_S32 h264e_csp_idx_map[RKV_H264E_CSP_BUTT];
#ifdef H264E_DUMP_DATA_TO_FILE
extern void hal_h264e_vpu_dump_mpp_strm_out(h264e_hal_context *ctx, MppBuffer hw_buf);
#endif
#if 0
extern MPP_RET hal_h264e_rkv_stream_init(h264e_hal_rkv_stream *s);
extern MPP_RET hal_h264e_rkv_set_sps(h264e_hal_sps *sps, h264e_hal_param *par, h264e_syntax *syn);
extern MPP_RET hal_h264e_rkv_sps_write(h264e_hal_sps *sps, h264e_hal_rkv_stream *s);
extern MPP_RET hal_h264e_rkv_set_pps(h264e_hal_pps *pps, h264e_hal_param *par, h264e_syntax *syn, h264e_hal_sps *sps);
extern MPP_RET hal_h264e_rkv_pps_write(h264e_hal_pps *pps, h264e_hal_sps *sps, h264e_hal_rkv_stream *s);
extern MPP_RET hal_h264e_rkv_stream_deinit(h264e_hal_rkv_stream *s);
extern void hal_h264e_rkv_nals_init(h264e_hal_rkv_out *out);
extern void hal_h264e_rkv_nals_deinit(h264e_hal_rkv_out *out);
extern void hal_h264e_rkv_nal_start(h264e_hal_rkv_out *out, RK_S32 i_type, RK_S32 i_ref_idc);
extern MPP_RET hal_h264e_rkv_encapsulate_nals(h264e_hal_rkv_out *out);
extern void hal_h264e_rkv_nal_end(h264e_hal_rkv_out *out);
#endif

typedef struct h264e_hal_test_cfg_t {
    RK_U32 hw_mode;
    char input_syntax_file_path[256];
    char input_yuv_file_path[256];
} h264e_hal_test_cfg;

static RK_U32 reg_idx2addr_map[132] = {
    0xffff, //0, unvalid.
    0x0000, //1
    0x0004, //2
    0x0008, //3
    0x000c, //4
    0x0010, //5
    0x0014, //6
    0x0018, //7
    0x001C, //8
    0x0030, //9
    0x0034, //10
    0x0038, //11
    0x003c, //12
    0x0040, //13
    0x0044, //14
    0x0048, //15
    0x004c, //16
    0x0050, //17
    0x0054, //18
    0x0058, //19
    0x005c, //20
    0x0060, //21.0~21.4
    0x0074, //22.0~22.39
    0x0114, //23
    0x0118, //24
    0x011c, //25
    0x0120, //26
    0x0124, //27
    0x0128, //28
    0x012c, //29
    0x0130, //30
    0x0134, //31
    0x0138, //32
    0x013c, //33
    0x0140, //34
    0x0144, //35
    0x0148, //36
    0x014c, //36
    0x0150, //38
    0x0154, //39
    0x0158, //40
    0x015c, //41
    0x0160, //42
    0x0164, //43
    0x0168, //44
    0x016c, //45
    0x0170, //46
    0x0174, //47
    0x0178, //48
    0x017c, //49
    0x0180, //50
    0x0184, //51
    0x0188, //52
    0x018c, //53
    0x0190, //54
    0x0194, //55
    0x0198, //56
    0x019c, //57
    0x01a0, //58
    0x01a4, //59
    0x01a8, //60
    0x01ac, //61
    0x01b0, //62
    0x01b4, //63
    0x01b8, //64
    0x01c0, //65
    0x01c4, //66
    0x01d0, //67.0~67.7
    0x01f0, //68.0~68.7
    0x0210, //69
    0x0214, //70
    0x0218, //71
    0x021c, //72
    0x0220, //73
    0x0224, //74
    0x0228, //75
    0x022c, //76
    0x0230, //77
    0x0234, //78
    0x0238, //79
    0x0400, //80.0~80.255
    0x0800, //81
    0x0804, //82
    0x0808, //83
    0x0810, //84
    0x0814, //85
    0x0818, //86
    0x0820, //87
    0x0824, //88
    0x0828, //89
    0x082c, //90
    0x0830, //91
    0x0834, //92
    0x0840, //93
    0x0844, //94
    0x0848, //95
    0x084c, //96
    0x0850, //97
    0x0854, //98
    0x0860, //99( 33 regs below are not present in c-model, included)
    0x0864, //100
    0x0868, //101
    0x086c, //102
    0x0870, //103
    0x0874, //104
    0x0880, //105
    0x0884, //106
    0x0888, //107
    0x088c, //108
    0x0890, //109
    0x0894, //110
    0x0898, //111
    0x089c, //112
    0x08a0, //113
    0x08a4, //114
    0x08a8, //115
    0x08ac, //116
    0x08b0, //117
    0x08b4, //118
    0x08b8, //119
    0x08bc, //120
    0x08c0, //121
    0x08c4, //122
    0x08c8, //123
    0x08cc, //124
    0x08d0, //125
    0x08d4, //126
    0x08d8, //127
    0x08dc, //128
    0x08e0, //129
    0x08e4, //130
    0x08e8, //131
};

static MPP_RET h264e_rkv_test_open_files(h264e_hal_test_cfg test_cfg)
{
    char base_path[512];
    char full_path[512];
#if H264E_HAL_FPGA_TEST
    strcpy(base_path, "/");
#else
    strcpy(base_path, "/sdcard/h264e_data/");
#endif

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

#if !H264E_HAL_FPGA_FAST_MODE
    sprintf(full_path, "%s%s", base_path, "mpp_syntax_in.txt");
    fp_mpp_syntax_in = fopen(full_path, "wb");
    if (!fp_mpp_syntax_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    sprintf(full_path, "%s%s", base_path, "mpp_yuv_in.yuv");
    fp_mpp_yuv_in = fopen(full_path, "wb");
    if (!fp_mpp_yuv_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    sprintf(full_path, "%s%s", base_path, "mpp_reg_in.txt");
    fp_mpp_reg_in = fopen(full_path, "wb");
    if (!fp_mpp_reg_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    sprintf(full_path, "%s%s", base_path, "mpp_reg_out.txt");
    fp_mpp_reg_out = fopen(full_path, "wb");
    if (!fp_mpp_reg_out) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    sprintf(full_path, "%s%s", base_path, "mpp_feedback.txt");
    fp_mpp_feedback = fopen(full_path, "wb");
    if (!fp_mpp_feedback) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

    sprintf(full_path, "%s%s", base_path, "mpp_wr2hw_reg_in.txt");
    fp_mpp_wr2hw_reg_in = fopen(full_path, "wb");
    if (!fp_mpp_syntax_in) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }
#endif //!H264E_HAL_FPGA_FAST_MODE

    sprintf(full_path, "%s%s", base_path, "mpp_strm_out.bin");
    fp_mpp_strm_out = fopen(full_path, "wb");
    if (!fp_mpp_strm_out) {
        mpp_err("%s open error", full_path);
        return MPP_ERR_OPEN_FILE;
    }

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

    read_ret = fread(hw_buf_ptr, 1, frame_size, fp_h264e_yuv_in);
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

static MPP_RET get_h264e_yuv_in_one_frame(RK_U8 *sw_buf, h264e_syntax *syn, MppBuffer *hw_buf)
{
    RK_U32 read_ret = 0;
    RK_U32 frame_luma_size = syn->pic_luma_width * syn->pic_luma_height;
    RK_U32 frame_size = frame_luma_size * 3 / 2;
    mpp_assert(fp_h264e_yuv_in);

    //TODO: remove sw_buf, read & write fd ptr directly
    read_ret = fread(sw_buf, 1, frame_size, fp_h264e_yuv_in);
    if (read_ret == 0) {
        mpp_log("yuv file end, nothing is read in frame %d", g_frame_cnt);
        return MPP_EOS_STREAM_REACHED;
    } else if (read_ret != frame_size) {
        mpp_err("read yuv one frame error, needed:%d, actual:%d", frame_size, read_ret);
        return MPP_NOK;
    } else {
        mpp_log("read frame %d size %d successfully", g_frame_cnt, frame_size);
    }


    mpp_buffer_write(hw_buf[0], 0, sw_buf,  frame_luma_size);
    mpp_buffer_write(hw_buf[1], 0, sw_buf + frame_luma_size, frame_luma_size / 4);
    mpp_buffer_write(hw_buf[2], 0, sw_buf + frame_luma_size * 5 / 4, frame_luma_size / 4);

    if (fp_mpp_yuv_in) {
        memcpy(sw_buf, mpp_buffer_get_ptr(hw_buf[0]), frame_luma_size);
        fwrite(sw_buf, 1, frame_luma_size, fp_mpp_yuv_in);
        memcpy(sw_buf, mpp_buffer_get_ptr(hw_buf[1]), frame_luma_size / 4);
        fwrite(sw_buf, 1, frame_luma_size / 4, fp_mpp_yuv_in);
        memcpy(sw_buf, mpp_buffer_get_ptr(hw_buf[2]), frame_luma_size / 4);
        fwrite(sw_buf, 1, frame_luma_size / 4, fp_mpp_yuv_in);
    }
    return MPP_OK;
}

static MPP_RET get_vpu_syntax_in(h264e_syntax *syn, MppBuffer *hw_in_buf, MppBuffer hw_output_strm_buf)
{
    RK_S32 k = 0;
    mpp_assert(fp_golden_syntax_in);
    memset(syn, 0, sizeof(h264e_syntax));

    if (hw_in_buf[0])
        syn->input_luma_addr = mpp_buffer_get_fd(hw_in_buf[0]);
    if (hw_in_buf[1])
        syn->input_cb_addr = mpp_buffer_get_fd(hw_in_buf[1]);
    if (hw_in_buf[2])
        syn->input_cr_addr = mpp_buffer_get_fd(hw_in_buf[2]);

    if (hw_output_strm_buf)
        syn->output_strm_addr = mpp_buffer_get_fd(hw_output_strm_buf);

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
        syn->color_conversion_r_mask_msb = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->color_conversion_g_mask_msb = data;

        fscanf(fp_golden_syntax_in, "%d", &data);
        fgets(temp, 512, fp_golden_syntax_in);
        syn->color_conversion_b_mask_msb = data;

        fgets(temp, 512, fp_golden_syntax_in);
    }

    return MPP_OK;
}




static void dump_rkv_mpp_syntax_in(h264e_hal_rkv_dbg_info *info, h264e_syntax *syn)
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


static void dump_rkv_mpp_reg_in( h264e_hal_context *ctx)
{
    RK_S32 k = 0;
    h264e_rkv_reg_set *reg_list = (h264e_rkv_reg_set *)ctx->regs;
    RK_U32 *regs = (RK_U32 *)&reg_list[ctx->frame_cnt_gen_ready - 1];

#if RKV_H264E_ADD_RESERVE_REGS
    h264e_rkv_reg_set * r = (h264e_rkv_reg_set *)regs;
#endif
    mpp_log("dump_rkv_mpp_reg_in enter, %d regs are dumped", RK_H264E_NUM_REGS);
    if (fp_mpp_reg_in) {
        FILE *fp = fp_mpp_reg_in;
        fprintf(fp, "#FRAME %d:\n", g_frame_cnt);
        //mpp_log("%d", r->swreg45.cime_rama_max);
        //mpp_log("%d", r->swreg45.cime_rama_h);
        //mpp_log("%d", r->swreg45.cach_l2_tag);
        //mpp_log("%d", r->swreg45.cach_l1_dtmr);
        //mpp_log("%d", r->swreg45.reserve);

        //mpp_log("%d", r->swreg57.nal_ref_idc);
        //mpp_log("%d", r->swreg57.nal_unit_type);
        //mpp_log("%d", r->swreg57.reserve);

        //mpp_log("%d", r->swreg59.etpy_mode);
        //mpp_log("%d", r->swreg59.trns_8x8);
        //mpp_log("%d", r->swreg59.csip_flg);
        //mpp_log("%d", r->swreg59.num_ref0_idx);
        //mpp_log("%d", r->swreg59.num_ref1_idx);
        //mpp_log("%d", r->swreg59.pic_init_qp);
        //mpp_log("%d", r->swreg59.cb_ofst);
        //mpp_log("%d", r->swreg59.cr_ofst);
        //mpp_log("%d", r->swreg59.wght_pred);
        //mpp_log("%d", r->swreg59.dbf_cp_flg);
        //mpp_log("%d", r->swreg59.reserve);

        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  1,  1, reg_idx2addr_map[ 1], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  2,  2, reg_idx2addr_map[ 2], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  3,  3, reg_idx2addr_map[ 3], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  4,  4, reg_idx2addr_map[ 4], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  5,  5, reg_idx2addr_map[ 5], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  6,  6, reg_idx2addr_map[ 6], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  7,  7, reg_idx2addr_map[ 7], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  8,  8, reg_idx2addr_map[ 8], *regs++);
#if RKV_H264E_ADD_RESERVE_REGS
        regs += MPP_ARRAY_ELEMS(r->reserve_08_09);
#endif

        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n",  9,  9, reg_idx2addr_map[ 9], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 10, 10, reg_idx2addr_map[10], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 11, 11, reg_idx2addr_map[11], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 12, 12, reg_idx2addr_map[12], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 13, 13, reg_idx2addr_map[13], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 14, 14, reg_idx2addr_map[14], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 15, 15, reg_idx2addr_map[15], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 16, 16, reg_idx2addr_map[16], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 17, 17, reg_idx2addr_map[17], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 18, 18, reg_idx2addr_map[18], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 19, 19, reg_idx2addr_map[19], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 20, 20, reg_idx2addr_map[20], *regs++);
        for (k = 0; k < 5; k++)
            fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 21, 21, reg_idx2addr_map[21], *regs++);
        for (k = 0; k < 40; k++)
            fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 22, 22, reg_idx2addr_map[22], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 23, 23, reg_idx2addr_map[23], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 24, 24, reg_idx2addr_map[24], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 25, 25, reg_idx2addr_map[25], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 26, 26, reg_idx2addr_map[26], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 27, 27, reg_idx2addr_map[27], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 28, 28, reg_idx2addr_map[28], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 29, 29, reg_idx2addr_map[29], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 30, 30, reg_idx2addr_map[30], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 31, 31, reg_idx2addr_map[31], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 32, 32, reg_idx2addr_map[32], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 33, 33, reg_idx2addr_map[33], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 34, 34, reg_idx2addr_map[34], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 35, 35, reg_idx2addr_map[35], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 36, 36, reg_idx2addr_map[36], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 37, 37, reg_idx2addr_map[37], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 38, 38, reg_idx2addr_map[38], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 39, 39, reg_idx2addr_map[39], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 40, 40, reg_idx2addr_map[40], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 41, 41, reg_idx2addr_map[41], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 42, 42, reg_idx2addr_map[42], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 43, 43, reg_idx2addr_map[43], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 44, 44, reg_idx2addr_map[44], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 45, 45, reg_idx2addr_map[45], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 46, 46, reg_idx2addr_map[46], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 47, 47, reg_idx2addr_map[47], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 48, 48, reg_idx2addr_map[48], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 49, 49, reg_idx2addr_map[49], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 50, 50, reg_idx2addr_map[50], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 51, 51, reg_idx2addr_map[51], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 52, 52, reg_idx2addr_map[52], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 53, 53, reg_idx2addr_map[53], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 54, 54, reg_idx2addr_map[54], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 55, 55, reg_idx2addr_map[55], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 56, 56, reg_idx2addr_map[56], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 57, 57, reg_idx2addr_map[57], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 58, 58, reg_idx2addr_map[58], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 59, 59, reg_idx2addr_map[59], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 60, 60, reg_idx2addr_map[60], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 61, 61, reg_idx2addr_map[61], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 62, 62, reg_idx2addr_map[62], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 63, 63, reg_idx2addr_map[63], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 64, 64, reg_idx2addr_map[64], *regs++);
#if RKV_H264E_ADD_RESERVE_REGS
        regs += MPP_ARRAY_ELEMS(r->reserve_64_65);
#endif

        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 65, 65, reg_idx2addr_map[65], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 66, 66, reg_idx2addr_map[66], *regs++);
#if RKV_H264E_ADD_RESERVE_REGS
        regs += MPP_ARRAY_ELEMS(r->reserve_66_67);
#endif

        for (k = 0; k < 8; k++)
            fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 67, 67, reg_idx2addr_map[67], *regs++);
        for (k = 0; k < 8; k++)
            fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 68, 68, reg_idx2addr_map[68], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 69, 69, reg_idx2addr_map[69], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 70, 70, reg_idx2addr_map[70], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 71, 71, reg_idx2addr_map[71], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 72, 72, reg_idx2addr_map[72], *regs++);

        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 74, 73, reg_idx2addr_map[73], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 75, 74, reg_idx2addr_map[74], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 76, 75, reg_idx2addr_map[75], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 77, 76, reg_idx2addr_map[76], *regs++);
#if !RKV_H264E_REMOVE_UNNECESSARY_REGS
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 78, 77, reg_idx2addr_map[77], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 79, 78, reg_idx2addr_map[78], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 80, 79, reg_idx2addr_map[79], *regs++);
#if RKV_H264E_ADD_RESERVE_REGS
        regs += MPP_ARRAY_ELEMS(r->reserve_79_80);
#endif

        for (k = 0; k < 256; k++)
            fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 73, 80, reg_idx2addr_map[80], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 81, 81, reg_idx2addr_map[81], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 82, 82, reg_idx2addr_map[82], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 83, 83, reg_idx2addr_map[83], *regs++);
#if RKV_H264E_ADD_RESERVE_REGS
        regs += MPP_ARRAY_ELEMS(r->reserve_83_84);
#endif

        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 84, 84, reg_idx2addr_map[84], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 85, 85, reg_idx2addr_map[85], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 86, 86, reg_idx2addr_map[86], *regs++);
#if RKV_H264E_ADD_RESERVE_REGS
        regs += MPP_ARRAY_ELEMS(r->reserve_86_87);
#endif

        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 87, 87, reg_idx2addr_map[87], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 88, 88, reg_idx2addr_map[88], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 89, 89, reg_idx2addr_map[89], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 90, 90, reg_idx2addr_map[90], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 91, 91, reg_idx2addr_map[91], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 92, 92, reg_idx2addr_map[92], *regs++);
#if RKV_H264E_ADD_RESERVE_REGS
        regs += MPP_ARRAY_ELEMS(r->reserve_92_93);
#endif

        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 93, 93, reg_idx2addr_map[93], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 94, 94, reg_idx2addr_map[94], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 95, 95, reg_idx2addr_map[95], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 96, 96, reg_idx2addr_map[96], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 97, 97, reg_idx2addr_map[97], *regs++);
        fprintf(fp, "reg[%03d/%03d/%04x]: %08x\n", 98, 98, reg_idx2addr_map[98], *regs  );
#endif //#if !RKV_H264E_REMOVE_UNNECESSARY_REGS


        fprintf(fp, "\n");
        fflush(fp);
    } else {
        mpp_log("try to dump data to mpp_reg_in.txt, but file is not opened");
    }
}

static void dump_rkv_mpp_reg_out(void *regs)
{
    RK_S32 k = 0;
    RK_U32 *reg = (RK_U32 *)regs;
    if (fp_mpp_reg_out) {
        fprintf(fp_mpp_reg_out, "#FRAME %d:\n", g_frame_cnt);
        for (k = 0; k < RK_H264E_NUM_REGS; k++) {
            fprintf(fp_mpp_reg_out, "reg[%03d/%03x]: %08x\n", k, k * 4, reg[k]);
            //mpp_log("reg[%03d/%03x]: %08x", k, k*4, reg[k]);
        }
        fprintf(fp_mpp_reg_out, "\n");
        fflush(fp_mpp_reg_out);
    } else {
        mpp_log("try to dump data to mpp_reg_out.txt, but file is not opened");
    }
}

static void dump_rkv_mpp_feedback(h264e_feedback *fb)
{
    (void)fb;
    if (fp_mpp_feedback) {

    } else {
        mpp_log("try to dump data to mpp_feedback.txt, but file is not opened");
    }
}

static void dump_rkv_mpp_wr2hw_reg_in(void *regs)
{
    //dump:   2~67,    80,       | total 372
    //left: 1,    68~79,  81~98  | total 38
    RK_S32 k = 0;
    RK_U32 *reg = (RK_U32 *)regs;
#if RKV_H264E_ADD_RESERVE_REGS
    h264e_rkv_reg_set * r = (h264e_rkv_reg_set *)regs;
#endif

    if (fp_mpp_wr2hw_reg_in) {
        FILE *fp = fp_mpp_wr2hw_reg_in;
        //fprintf(fp_mpp_reg_out, "#FRAME %d:\n", g_frame_cnt);
        RK_S32 ofs_9_64 = 0;
        RK_S32 ofs_65_66 = 0;
        RK_S32 ofs_67_79 = 0;
        RK_S32 ofs_80_83 = 0;
        //RK_S32 ofs_84_86 = 0;
        //RK_S32 ofs_87_92 = 0;
        //RK_S32 ofs_93_98 = 0;
#if RKV_H264E_ADD_RESERVE_REGS
        ofs_9_64 = MPP_ARRAY_ELEMS(r->reserve_08_09);
        ofs_65_66 = ofs_9_64 + MPP_ARRAY_ELEMS(r->reserve_64_65);
        ofs_67_79 = ofs_65_66 + MPP_ARRAY_ELEMS(r->reserve_66_67);
#if !RKV_H264E_REMOVE_UNNECESSARY_REGS
        ofs_80_83 = ofs_67_79 + MPP_ARRAY_ELEMS(r->reserve_79_80);
        //ofs_84_86 = ofs_80_83 + MPP_ARRAY_ELEMS(r->reserve_83_84);
        //ofs_87_92 = ofs_84_86 + MPP_ARRAY_ELEMS(r->reserve_86_87);
        //ofs_93_98 = ofs_87_92 + MPP_ARRAY_ELEMS(r->reserve_92_93);
#endif
#endif
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[24], reg[66 + ofs_9_64]); //0
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[25], reg[67 + ofs_9_64]); //1
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[26], reg[68 + ofs_9_64]); //2
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[27], reg[69 + ofs_9_64]); //3
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[28], reg[70 + ofs_9_64]); //4
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[29], reg[71 + ofs_9_64]); //5
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[30], reg[72 + ofs_9_64]); //6
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[31], reg[73 + ofs_9_64]); //7
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[32], reg[74 + ofs_9_64]); //8
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[33], reg[75 + ofs_9_64]); //9
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[34], reg[76 + ofs_9_64]); //10
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[35], reg[77 + ofs_9_64]); //11
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[36], reg[78 + ofs_9_64]); //12
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[37], reg[79 + ofs_9_64]); //13
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[38], reg[80 + ofs_9_64]); //14

        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[40], reg[82 + ofs_9_64]); //15
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[39], reg[81 + ofs_9_64]); //16

        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[3 ], reg[2 ]); //17
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[4 ], reg[3 ]); //18
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[5 ], reg[4 ]); //19
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[6 ], reg[5 ]); //20
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[7 ], reg[6 ]); //21
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[8 ], reg[7 ]); //22
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[9 ], reg[8 + ofs_9_64]); //23
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[10], reg[9 + ofs_9_64]); //24
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[11], reg[10 + ofs_9_64]); //25
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[12], reg[11 + ofs_9_64]); //26
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[13], reg[12 + ofs_9_64]); //27
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[14], reg[13 + ofs_9_64]); //28
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[15], reg[14 + ofs_9_64]); //29
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[16], reg[15 + ofs_9_64]); //30
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[17], reg[16 + ofs_9_64]); //31
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[18], reg[17 + ofs_9_64]); //32
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[19], reg[18 + ofs_9_64]); //33
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[20], reg[19 + ofs_9_64]); //34

        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[23], reg[65 + ofs_9_64]); //35

        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[41], reg[83 + ofs_9_64]); //36
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[42], reg[84 + ofs_9_64]); //37
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[43], reg[85 + ofs_9_64]); //38
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[44], reg[86 + ofs_9_64]); //39
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[45], reg[87 + ofs_9_64]); //40
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[46], reg[88 + ofs_9_64]); //41
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[47], reg[89 + ofs_9_64]); //42
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[48], reg[90 + ofs_9_64]); //43
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[49], reg[91 + ofs_9_64]); //44
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[50], reg[92 + ofs_9_64]); //45
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[51], reg[93 + ofs_9_64]); //46
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[52], reg[94 + ofs_9_64]); //47
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[53], reg[95 + ofs_9_64]); //48
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[54], reg[96 + ofs_9_64]); //49
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[55], reg[97 + ofs_9_64]); //50
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[56], reg[98 + ofs_9_64]); //51
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[57], reg[99 + ofs_9_64]); //52
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[58], reg[100 + ofs_9_64]); //53
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[59], reg[101 + ofs_9_64]); //54
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[60], reg[102 + ofs_9_64]); //55
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[61], reg[103 + ofs_9_64]); //56
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[62], reg[104 + ofs_9_64]); //57
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[63], reg[105 + ofs_9_64]); //58
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[64], reg[106 + ofs_9_64]); //59
        for (k = 0; k < 5; k++)
            fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[21] + k * 4, reg[20 + k + ofs_9_64]); //60~64
        for (k = 0; k < 40; k++)
            fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[22] + k * 4, reg[25 + k + ofs_9_64]); //65~104
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[65], reg[107 + ofs_65_66]); //105
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[66], reg[108 + ofs_65_66]); //106
        for (k = 0; k < 8; k++)
            fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[67] + 4 * k, reg[109 + k + ofs_67_79]); //107~114
#if !RKV_H264E_REMOVE_UNNECESSARY_REGS
        for (k = 0; k < 256; k++)
            fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[80] + 4 * k, reg[136 + k + ofs_80_83]); //115~370
#endif
        fprintf(fp, "0x%08x 0x%08x,\n", reg_idx2addr_map[2], reg[1]); //371

        (void)ofs_80_83;
        fflush(fp);
    } else {
        mpp_log("try to dump data to mpp_reg_out.txt, but file is not opened");
    }
}

static void dump_rkv_mpp_strm_out_header(h264e_control_extra_info *extra_info)
{
    if (fp_mpp_strm_out) {
        FILE *fp = fp_mpp_strm_out;
        fwrite(extra_info->buf, 1, extra_info->size, fp);
        fflush(fp);
    } else {
        mpp_log("try to dump strm header to mpp_strm_out.txt, but file is not opened");
    }
}


static MPP_RET get_rkv_syntax_in(h264e_hal_rkv_dbg_info *info, h264e_syntax *syn,
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

    syn->keyframe_max_interval = 4;

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




static void dump_rkv_mpp_strm_out(MppBuffer *hw_buf, void *ioctl_output)
{
    if (fp_mpp_strm_out) {
        RK_U32 k = 0;
        RK_U32 strm_size = 0;
        RK_U8 *sw_buf = NULL;
        RK_U8 *hw_buf_vir_addr = NULL;
        h264e_rkv_ioctl_output *out = (h264e_rkv_ioctl_output *)ioctl_output;
        h264e_rkv_ioctl_output_elem *out_elem = out->elem;
        RK_U32 frame_num = out->frame_num;

        mpp_log("dump %d frames strm out below", frame_num);
        for (k = 0; k < frame_num; k++) {
            strm_size = (RK_U32)out_elem[k].swreg69.bs_lgth;
            hw_buf_vir_addr = (RK_U8 *)mpp_buffer_get_ptr(hw_buf[k]);
            sw_buf = mpp_malloc(RK_U8, strm_size);

            mpp_log("dump frame %d strm_size: %d", k, strm_size);

            memcpy(sw_buf, hw_buf_vir_addr, strm_size);

            fwrite(sw_buf, 1, strm_size, fp_mpp_strm_out);

            if (sw_buf)
                mpp_free(sw_buf);
        }
        fflush(fp_mpp_strm_out);


        (void)hw_buf;
    } else {
        mpp_log("try to dump data to mpp_strm_out.txt, but file is not opened");
    }
}

static MPP_RET h264e_hal_test_parse_options(int arg_num, char **arg_str, h264e_hal_test_cfg *cfg)
{
    RK_S32 k = 0;
    memset(cfg, 0, sizeof(h264e_hal_test_cfg));

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
    }


    if (!cfg->input_yuv_file_path) {
        mpp_log("test param parse error: input_yuv_file_path is NULL");
        return MPP_NOK;
    }

    if (!cfg->input_syntax_file_path) {
        mpp_log("test param parse error: input_syntax_file_path is NULL");
        return MPP_NOK;
    }

    if (cfg->hw_mode == 0)
        test_device_id = HAL_RKVENC;
    else if (cfg->hw_mode == 1)
        test_device_id = HAL_VEPU;
    else {
        mpp_log("test param parse error: hw_mode is %d", cfg->hw_mode);
        return MPP_NOK;
    }

    return MPP_OK;
}

static void h264e_hal_test_init(h264e_hal_context *ctx, HalTaskInfo *task_info, h264e_syntax *syn)
{
    memset(ctx, 0, sizeof(h264e_hal_context));
    memset(task_info, 0, sizeof(HalTaskInfo));
    task_info->enc.syntax.data = (void *)syn;

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
}

MPP_RET h264e_hal_vpu_test()
{
    RK_S32 k = 0;
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
    MppBuffer hw_input_buf[3] = {NULL}; //Y, U, V
    MppBuffer hw_output_strm_buf = NULL;
    RK_U32 frame_luma_stride = 0;
    struct timeval t0;

    RK_U8 *input_sw_buf = mpp_malloc(RK_U8, MAX_FRAME_TOTAL_SIZE);

    mpp_packet_init(&extra_info_pkt, (void *)extra_info_buf, H264E_MAX_PACKETED_PARAM_SIZE);
    
    get_vpu_syntax_in(&syntax_data, hw_input_buf, hw_output_strm_buf);
    fseek(fp_golden_syntax_in, 0L, SEEK_SET);

    frame_luma_stride = ((syntax_data.pic_luma_width + 15) & (~15)) * ((syntax_data.pic_luma_height + 15) & (~15));

    h264e_hal_test_init(&ctx, &task_info, &syntax_data);

    if (MPP_OK != mpp_buffer_group_get_internal(&hw_input_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_input_buf_grp get failed, test is ended early");
        goto __test_end;
    }
    if (MPP_OK != mpp_buffer_group_get_internal(&hw_output_buf_grp, MPP_BUFFER_TYPE_ION)) {
        mpp_err("hw_output_buf_grp get failed, test is ended early");
        goto __test_end;
    }

    mpp_buffer_get(hw_input_buf_grp, &hw_input_buf[0], frame_luma_stride);
    mpp_buffer_get(hw_input_buf_grp, &hw_input_buf[1], frame_luma_stride / 4); // TODO: according to yuv format
    mpp_buffer_get(hw_input_buf_grp, &hw_input_buf[2], frame_luma_stride / 4);
    mpp_buffer_get(hw_output_buf_grp, &hw_output_strm_buf, 1024 * 1024 * 2);

    hal_cfg.hal_int_cb.callBack = h264_hal_test_call_back;
    hal_cfg.hal_int_cb.opaque = NULL; //control context
    hal_cfg.device_id = test_device_id;
    hal_h264e_init(&ctx, &hal_cfg);


    h264e_hal_set_extra_info_cfg(&extra_info_cfg, &syntax_data);
    hal_h264e_vpu_control(&ctx, MPP_ENC_SET_EXTRA_INFO, &extra_info_cfg);
    hal_h264e_vpu_control(&ctx, MPP_ENC_GET_EXTRA_INFO, &extra_info_pkt);


    do {
        /* get golden input */
        if (MPP_EOS_STREAM_REACHED == get_vpu_syntax_in(&syntax_data, hw_input_buf, hw_output_strm_buf)) {
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
        gettimeofday(&t0, NULL);
        mpp_log("hal_h264e_start time : %d ", ((long)t0.tv_sec) * 1000 + (long)t0.tv_usec / 1000);

        hal_h264e_start(&ctx, &task_info);
        hal_h264e_wait(&ctx, &task_info);

#ifdef H264E_DUMP_DATA_TO_FILE
        hal_h264e_vpu_dump_mpp_strm_out(&ctx, hw_output_strm_buf);
#endif

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
    for (k = 0; k < 3; k++) {
        if (hw_input_buf[k])
            mpp_buffer_put(hw_input_buf[k]);
    }
    if (hw_output_strm_buf)
        mpp_buffer_put(hw_output_strm_buf);

    if (hw_input_buf_grp)
        mpp_buffer_group_put(hw_input_buf_grp);
    if (hw_output_buf_grp)
        mpp_buffer_group_put(hw_output_buf_grp);

    return MPP_OK;
}


MPP_RET h264e_hal_rkv_test()
{
    RK_S32 k = 0;
    MPP_RET ret = MPP_OK;
    h264e_hal_context ctx;
    MppHalCfg hal_cfg;
    HalTaskInfo task_info;
    h264e_control_extra_info_cfg extra_info_cfg;
    h264e_control_extra_info extra_info;
    h264e_hal_rkv_dbg_info dbg_info;
    h264e_syntax syntax_data[RKV_H264E_LINKTABLE_FRAME_NUM];
    MppBufferGroup hw_input_buf_grp = NULL;
    MppBufferGroup hw_output_buf_grp = NULL;
    MppBuffer hw_input_buf_mul[RKV_H264E_LINKTABLE_FRAME_NUM] = {NULL};
    MppBuffer hw_output_strm_buf_mul[RKV_H264E_LINKTABLE_FRAME_NUM] = {NULL};
    RK_U32 frame_luma_stride = 0;
    struct timeval t0;

    get_rkv_syntax_in(&dbg_info, &syntax_data[0], hw_input_buf_mul, hw_output_strm_buf_mul);
    fseek(fp_golden_syntax_in, 0L, SEEK_SET);


    frame_luma_stride = ((syntax_data[0].pic_luma_width + 15) & (~15)) * ((syntax_data[0].pic_luma_height + 15) & (~15));

    h264e_hal_test_init(&ctx, &task_info, syntax_data);


    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++)
        mpp_buffer_get(hw_input_buf_grp, &hw_input_buf_mul[k], frame_luma_stride * 3 / 2);

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++)
        mpp_buffer_get(hw_output_buf_grp, &hw_output_strm_buf_mul[k], 1024 * 1024 * 2);


    hal_cfg.hal_int_cb.callBack = h264_hal_test_call_back;
    hal_cfg.hal_int_cb.opaque = NULL; //control context
    hal_cfg.device_id = test_device_id;
    hal_h264e_init(&ctx, &hal_cfg);

    h264e_hal_set_extra_info_cfg(&extra_info_cfg, &syntax_data[0]); //TODO: use dbg info for input instead
    hal_h264e_rkv_control(&ctx, MPP_ENC_SET_EXTRA_INFO, &extra_info_cfg);
    hal_h264e_rkv_control(&ctx, MPP_ENC_GET_EXTRA_INFO, &extra_info);
    dump_rkv_mpp_strm_out_header(&extra_info);

    do {
        /* get golden input */
        if (g_frame_read_cnt <= g_frame_cnt) {
            h264e_syntax *syn = NULL;
            RK_S32 frame_num = RKV_H264E_ENC_MODE == 1 ? 1 : RKV_H264E_LINKTABLE_FRAME_NUM;
            mpp_log("read %d frames input", frame_num);
            for (k = 0; k < frame_num; k++, g_frame_read_cnt++) {
                syn = &syntax_data[g_frame_read_cnt % RKV_H264E_LINKTABLE_FRAME_NUM];
                if (MPP_EOS_STREAM_REACHED == get_rkv_syntax_in(&dbg_info, syn, hw_input_buf_mul, hw_output_strm_buf_mul)) {
                    mpp_log("syntax input file end, total %d frames are encoded, test is ended", g_frame_cnt);
                    goto __test_end;
                }
                dump_rkv_mpp_syntax_in(&dbg_info, syn);
                ret = get_rkv_h264e_yuv_in_frame(syn, hw_input_buf_mul);
                if (ret == MPP_EOS_STREAM_REACHED) {
                    mpp_log("yuv file end, total %d frames are encoded, test is ended", g_frame_cnt);
                    break;
                } else if (ret == MPP_NOK) {
                    mpp_err("read yuv file failed, test is ended early");
                    goto __test_end;
                }
            }
        }

        task_info.enc.syntax.data = (void *)&syntax_data[g_frame_cnt % RKV_H264E_LINKTABLE_FRAME_NUM];

        /* generate registers */
        hal_h264e_gen_regs(&ctx, &task_info);

        dump_rkv_mpp_reg_in(&ctx);
        dump_rkv_mpp_wr2hw_reg_in(ctx.regs);

        /* run hardware */
        gettimeofday(&t0, NULL);
        mpp_log("hal_h264e_start time : %d ", ((long)t0.tv_sec) * 1000 + (long)t0.tv_usec / 1000);

        hal_h264e_start(&ctx, &task_info);
        hal_h264e_wait(&ctx, &task_info);

        dump_rkv_mpp_reg_out(ctx.regs);
        dump_rkv_mpp_feedback(&ctx.feedback);

        g_frame_cnt ++;

        if (g_frame_cnt == g_frame_read_cnt)
            dump_rkv_mpp_strm_out(hw_output_strm_buf_mul, ctx.ioctl_output);
    } while (1);


__test_end:
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
        h264e_hal_rkv_test();
    }

__test_end:

    h264e_test_close_files();

    mpp_log("******* h264e hal test end *******");
    return 0;
}
