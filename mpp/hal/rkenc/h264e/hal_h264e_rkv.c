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

#define MODULE_TAG "hal_h264e_rkv"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "vpu.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "h264_syntax.h"
#include "hal_h264e_com.h"
#include "hal_h264e_rkv.h"

#define RKVENC_DUMP_INFO                 0

#define RKVENC_CODING_TYPE_AUTO          0x0000  /* Let x264 choose the right type */
#define RKVENC_CODING_TYPE_IDR           0x0001
#define RKVENC_CODING_TYPE_I             0x0002
#define RKVENC_CODING_TYPE_P             0x0003
#define RKVENC_CODING_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define RKVENC_CODING_TYPE_B             0x0005
#define RKVENC_CODING_TYPE_KEYFRAME      0x0006  /* IDR or I depending on b_open_gop option */
#define RKVENC_IS_TYPE_I(x) ((x)==RKVENC_CODING_TYPE_I || (x)==RKVENC_CODING_TYPE_IDR)
#define RKVENC_IS_TYPE_B(x) ((x)==RKVENC_CODING_TYPE_B || (x)==RKVENC_CODING_TYPE_BREF)
#define RKVENC_IS_DISPOSABLE(type) ( type == RKVENC_CODING_TYPE_B )

#define H264E_IOC_CUSTOM_BASE           0x1000
#define H264E_IOC_SET_OSD_PLT           (H264E_IOC_CUSTOM_BASE + 1)

static const RK_U32 h264e_h3d_tbl[40] = {
    0x0b080400, 0x1815120f, 0x23201e1b, 0x2c2a2725,
    0x33312f2d, 0x38373634, 0x3d3c3b39, 0x403f3e3d,
    0x42414140, 0x43434342, 0x44444444, 0x44444444,
    0x44444444, 0x43434344, 0x42424343, 0x40414142,
    0x3d3e3f40, 0x393a3b3c, 0x35363738, 0x30313334,
    0x2c2d2e2f, 0x28292a2b, 0x23242526, 0x20202122,
    0x191b1d1f, 0x14151618, 0x0f101112, 0x0b0c0d0e,
    0x08090a0a, 0x06070708, 0x05050506, 0x03040404,
    0x02020303, 0x01010102, 0x00010101, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
};

static const RK_U8 h264e_ue_size_tab[256] = {
    1, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
};

#if RKVENC_DUMP_INFO
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
#endif

static H264eRkvMbRcMcfg mb_rc_m_cfg[H264E_MB_RC_M_NUM] = {
    /* aq_prop, aq_strength, mb_num, qp_range */
    {16,        1,           0,      1}, // mode = 0
    {8,         1,           1,      2}, // mode = 1
    {4,         1,           1,      4}, // mode = 2
    {2,         1,           1,      4}, // mode = 3
    {0,         1,           1,      2}, // mode = 4
};

static H264eRkvMbRcQcfg mb_rc_q_cfg[MPP_ENC_RC_QUALITY_BUTT] = {
    /* qp_min, qp_max */
    {31,       51}, // worst
    {28,       46}, // worse
    {24,       42}, // medium
    {20,       39}, // better
    {16,       35}, // best
    {0,         0}, // cqp
};

static MPP_RET h264e_rkv_close_dump_files(void *dump_files)
{
    H264eRkvDumpFiles *files = (H264eRkvDumpFiles *)dump_files;
    H264E_HAL_FCLOSE(files->fp_mpp_syntax_in);
    H264E_HAL_FCLOSE(files->fp_mpp_reg_in);
    H264E_HAL_FCLOSE(files->fp_mpp_reg_out);
    H264E_HAL_FCLOSE(files->fp_mpp_strm_out);
    H264E_HAL_FCLOSE(files->fp_mpp_feedback);
    H264E_HAL_FCLOSE(files->fp_mpp_extra_ino_cfg);
    return MPP_OK;
}

static double QP2Qstep( double qp_avg )
{
    RK_S32 i;
    double Qstep;
    RK_S32 QP = qp_avg * 4.0;

    Qstep = 0.625 * pow(1.0293, QP % 24);
    for (i = 0; i < (QP / 24); i++)
        Qstep *= 2;

    return round(Qstep * 4);
}

static MPP_RET h264e_rkv_open_dump_files(void *dump_files)
{
    if (h264e_hal_log_mode & H264E_DBG_FILE) {
        char base_path[512];
        char full_path[512];
        H264eRkvDumpFiles *files = (H264eRkvDumpFiles *)dump_files;
        strcpy(base_path, "/tmp/");

        sprintf(full_path, "%s%s", base_path, "mpp_syntax_in.txt");
        files->fp_mpp_syntax_in = fopen(full_path, "wb");
        if (!files->fp_mpp_syntax_in) {
            h264e_hal_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }


        sprintf(full_path, "%s%s", base_path, "mpp_reg_in.txt");
        files->fp_mpp_reg_in = fopen(full_path, "wb");
        if (!files->fp_mpp_reg_in) {
            h264e_hal_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_reg_out.txt");
        files->fp_mpp_reg_out = fopen(full_path, "wb");
        if (!files->fp_mpp_reg_out) {
            h264e_hal_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_feedback.txt");
        files->fp_mpp_feedback = fopen(full_path, "wb");
        if (!files->fp_mpp_feedback) {
            h264e_hal_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_strm_out.bin");
        files->fp_mpp_strm_out = fopen(full_path, "wb");
        if (!files->fp_mpp_strm_out) {
            h264e_hal_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }

        sprintf(full_path, "%s%s", base_path, "mpp_extra_info_cfg.txt");
        files->fp_mpp_extra_ino_cfg = fopen(full_path, "wb");
        if (!files->fp_mpp_extra_ino_cfg) {
            h264e_hal_err("%s open error", full_path);
            return MPP_ERR_OPEN_FILE;
        }
    }
    return MPP_OK;
}

static void h264e_rkv_dump_mpp_syntax_in(H264eHwCfg *syn, H264eHalContext *ctx)
{
#if RKVENC_DUMP_INFO
    H264eRkvDumpFiles *dump_files = (H264eRkvDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_syntax_in;
    if (fp) {
        //RK_S32 k = 0;
        fprintf(fp, "#FRAME %d\n", ctx->frame_cnt);

        fprintf(fp, "%-16d %s\n", syn->width, "pic_luma_width");
        fprintf(fp, "%-16d %s\n", syn->height, "pic_luma_height");
        fprintf(fp, "%-16d %s\n", syn->coding_type, "frame_coding_type");
        fprintf(fp, "%-16d %s\n", syn->qp, "swreg10.pic_qp");
        fprintf(fp, "%-16d %s\n", syn->input_format, "swreg14.src_cfmt");

        fprintf(fp, "%-16d %s\n", syn->frame_type, "swreg60.sli_type");
        fprintf(fp, "%-16d %s\n", syn->frame_num, "swreg60.frm_num");

        fprintf(fp, "%-16d %s\n", syn->idr_pic_id, "swreg61.idr_pid");
        fprintf(fp, "%-16d %s\n", syn->pic_order_cnt_lsb, "swreg61.poc_lsb");

        fprintf(fp, "%-16d %s\n", syn->keyframe_max_interval, "keyframe_max_interval");

        fprintf(fp, "\n");
        fflush(fp);
    } else {
        h264e_hal_dbg(H264E_DBG_FILE, "try to dump data to mpp_syntax_in.txt, but file is not opened");
    }
#else
    (void)ctx;
    (void)syn;
#endif
}

static void h264e_rkv_dump_mpp_reg_in(H264eHalContext *ctx)
{
#if RKVENC_DUMP_INFO
    RK_S32 k = 0;
    H264eRkvDumpFiles *dump_files = (H264eRkvDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_reg_in;
    H264eRkvRegSet *reg_list = (H264eRkvRegSet *)ctx->regs;
    RK_U32 *regs = (RK_U32 *)&reg_list[ctx->frame_cnt_gen_ready];

#if RKV_H264E_ADD_RESERVE_REGS
    H264eRkvRegSet * r = (H264eRkvRegSet *)regs;
#endif
    h264e_hal_dbg(H264E_DBG_FILE, "dump_rkv_mpp_reg_in enter, %d regs are dumped", RK_H264E_NUM_REGS);
    if (fp) {
        fprintf(fp, "#FRAME %d:\n", ctx->frame_cnt);

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

        fprintf(fp, "\n");
        fflush(fp);
    } else {
        h264e_hal_dbg(H264E_DBG_FILE, "try to dump data to mpp_reg_in.txt, but file is not opened");
    }
#else
    (void)ctx;
#endif
}

static void h264e_rkv_dump_mpp_reg_out(H264eHalContext *ctx)
{
#if RKVENC_DUMP_INFO
    RK_U32 k = 0;
    H264eRkvDumpFiles *dump_files = (H264eRkvDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_reg_out;
    H264eRkvIoctlOutput *reg_out = (H264eRkvIoctlOutput *)ctx->ioctl_output;
    RK_U32 *p_reg = (RK_U32 *)reg_out + sizeof(reg_out->frame_num) / 4;
    if (fp) {
        fprintf(fp, "%d frames out\n", reg_out->frame_num);
        for (k = 0; k < ctx->num_frames_to_send; k++) {
            fprintf(fp, "#FRAME %d:\n", (ctx->frame_cnt - 1) - (ctx->num_frames_to_send - 1 - k));
            for (k = 0; k < 12; k++) {
                fprintf(fp, "reg[%03d/%03x]: %08x\n", k, k * 4, p_reg[k]);
            }
            fprintf(fp, "\n");
            p_reg += sizeof(reg_out->elem[0]) / 4;
        }
        fflush(fp);
    } else {
        h264e_hal_dbg(H264E_DBG_FILE, "try to dump data to mpp_reg_out.txt, but file is not opened");
    }
#else
    (void)ctx;
#endif
}

static void h264e_rkv_dump_mpp_feedback(H264eHalContext *ctx)
{
#if RKVENC_DUMP_INFO
    H264eRkvDumpFiles *dump_files = (H264eRkvDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_feedback;
    if (fp) {
        h264e_feedback *fb = &ctx->feedback;
        (void)fb;
    } else {
        h264e_hal_dbg(H264E_DBG_FILE,
                      "try to dump data to mpp_feedback.txt, but file is not opened");
    }
#else
    (void)ctx;
#endif
}

static void
h264e_rkv_dump_mpp_strm_out_header(H264eHalContext *ctx, MppPacket packet)
{
#if RKVENC_DUMP_INFO
    H264eRkvDumpFiles *dump_files = (H264eRkvDumpFiles *)ctx->dump_files;
    void *ptr   = mpp_packet_get_data(packet);
    size_t len  = mpp_packet_get_length(packet);
    FILE *fp = dump_files->fp_mpp_strm_out;

    if (fp) {
        fwrite(ptr, 1, len, fp);
        fflush(fp);
    } else {
        h264e_hal_dbg(H264E_DBG_FILE,
                      "try to dump strm header to mpp_strm_out.txt, but file is not opened");
    }
#else
    (void)ctx;
    (void)packet;
#endif
}


static void h264e_rkv_dump_mpp_strm_out(H264eHalContext *ctx, MppBuffer hw_buf)
{
#if RKVENC_DUMP_INFO
    H264eRkvDumpFiles *dump_files = (H264eRkvDumpFiles *)ctx->dump_files;
    FILE *fp = dump_files->fp_mpp_strm_out;
    if (fp) {
        RK_U32 k = 0;
        RK_U32 strm_size = 0;
        RK_U8 *sw_buf = NULL;
        RK_U8 *hw_buf_vir_addr = NULL;
        H264eRkvIoctlOutput *ioctl_output =
            (H264eRkvIoctlOutput *)ctx->ioctl_output;
        H264eRkvIoctlOutputElem *out_elem = ioctl_output->elem;
        RK_U32 frame_num = ioctl_output->frame_num;

        h264e_hal_dbg(H264E_DBG_FILE, "dump %d frames strm out below",
                      frame_num);
        for (k = 0; k < frame_num; k++) {
            strm_size = (RK_U32)out_elem[k].swreg69.bs_lgth;
            hw_buf_vir_addr = (RK_U8 *)mpp_buffer_get_ptr(hw_buf);
            sw_buf = mpp_malloc(RK_U8, strm_size);

            h264e_hal_dbg(H264E_DBG_FILE, "dump frame %d, fd %d, strm_size: %d",
                          k, mpp_buffer_get_fd(hw_buf), strm_size);

            memcpy(sw_buf, hw_buf_vir_addr, strm_size);

            fwrite(sw_buf, 1, strm_size, fp);

            if (sw_buf)
                mpp_free(sw_buf);
        }
        fflush(fp);
    } else {
        h264e_hal_dbg(H264E_DBG_FILE,
                      "try to dump data to mpp_strm_out.txt, but file is not opened");
    }
#else
    (void)ctx;
    (void)hw_buf;
#endif
}

static void h264e_rkv_frame_push( H264eRkvFrame **list, H264eRkvFrame *frame )
{
    RK_S32 i = 0;
    while ( list[i] ) i++;
    list[i] = frame;
    h264e_hal_dbg(H264E_DBG_DPB, "frame push list[%d] %p", i, frame);

}

static H264eRkvFrame *h264e_rkv_frame_new(H264eRkvDpbCtx *dpb_ctx)
{
    RK_S32 k = 0;
    H264eRkvFrame *frame_buf = dpb_ctx->frame_buf;
    RK_S32 num_buf = MPP_ARRAY_ELEMS(dpb_ctx->frame_buf);
    H264eRkvFrame *new_frame = NULL;
    h264e_hal_enter();
    for (k = 0; k < num_buf; k++) {
        if (!frame_buf[k].hw_buf_used) {
            new_frame = &frame_buf[k];
            frame_buf[k].hw_buf_used = 1;
            break;
        }
    }

    if (!new_frame) {
        h264e_hal_err("!new_frame, new_frame get failed");
        return NULL;
    }

    h264e_hal_leave();
    return new_frame;
}

static H264eRkvFrame *h264e_rkv_frame_pop( H264eRkvFrame **list )
{
    H264eRkvFrame *frame;
    RK_S32 i = 0;
    mpp_assert( list[0] );
    while ( list[i + 1] ) i++;
    frame = list[i];
    list[i] = NULL;
    h264e_hal_dbg(H264E_DBG_DPB, "frame pop list[%d] %p", i, frame);
    return frame;
}

static void
h264e_rkv_frame_unshift(H264eRkvFrame **list, H264eRkvFrame *frame)
{
    RK_S32 i = 0;
    while ( list[i] ) i++;
    while ( i-- )
        list[i + 1] = list[i];
    list[0] = frame;
    h264e_hal_dbg(H264E_DBG_DPB, "frame unshift list[0] %p", frame);
}

static H264eRkvFrame *h264e_rkv_frame_shift( H264eRkvFrame **list )
{
    H264eRkvFrame *frame = list[0];
    RK_S32 i;
    for ( i = 0; list[i]; i++ )
        list[i] = list[i + 1];
    mpp_assert(frame);
    h264e_hal_dbg(H264E_DBG_DPB, "frame shift list[0] %p", frame);
    return frame;
}


static void
h264e_rkv_frame_push_unused( H264eRkvDpbCtx *dpb_ctx, H264eRkvFrame *frame)
{
    h264e_hal_enter();
    mpp_assert( frame->i_reference_count > 0 );
    frame->i_reference_count--;
    if ( frame->i_reference_count == 0 )
        h264e_rkv_frame_push( dpb_ctx->frames.unused, frame );
    h264e_hal_leave();
}

static H264eRkvFrame *h264e_rkv_frame_pop_unused( H264eHalContext *ctx)
{
    H264eRkvFrame *frame = NULL;

    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;

    h264e_hal_enter();
    if ( dpb_ctx->frames.unused[0] )
        frame = h264e_rkv_frame_pop( dpb_ctx->frames.unused );
    else {
        frame = h264e_rkv_frame_new( dpb_ctx );
    }

    if ( !frame ) {
        h264e_hal_err("!frame, return NULL");
        return NULL;
    }
    frame->i_reference_count = 1;
    frame->b_keyframe = 0;
    frame->b_corrupt = 0;
    frame->long_term_flag = 0;
    frame->reorder_longterm_flag = 0;

    h264e_hal_leave();

    return frame;
}

static void h264e_rkv_reference_reset( H264eRkvDpbCtx *dpb_ctx )
{
    h264e_hal_enter();
    while ( dpb_ctx->frames.reference[0] )
        h264e_rkv_frame_push_unused( dpb_ctx, h264e_rkv_frame_pop( dpb_ctx->frames.reference ) );
    dpb_ctx->fdec->i_poc = 0;
    h264e_hal_leave();
}

#if 0
static void h264e_rkv_reference_hierarchy_reset( H264eHalContext *ctx)
{
    RK_S32 ref;
    RK_S32 i = 0;
    RK_S32 b_hasdelayframe = 0;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    RK_S32 i_num_reorder_frames = sps->vui.i_num_reorder_frames;

    /* look for delay frames -- chain must only contain frames that are disposable */
    for ( i = 0; dpb_ctx->frames.current[i] && RKVENC_IS_DISPOSABLE( dpb_ctx->frames.current[i]->i_type ); i++ )
        b_hasdelayframe |= dpb_ctx->frames.current[i]->i_coded
                           != dpb_ctx->frames.current[i]->i_frame + i_num_reorder_frames;

    /* This function must handle b-pyramid and clear frames for open-gop */
    if (!b_hasdelayframe && dpb_ctx->frames.i_poc_last_open_gop == -1 )
        return;

    /* Remove last BREF. There will never be old BREFs in the
     * dpb during a BREF decode when pyramid == STRICT */
    for ( ref = 0; dpb_ctx->frames.reference[ref]; ref++ ) {
        if (dpb_ctx->frames.reference[ref]->i_poc < dpb_ctx->frames.i_poc_last_open_gop &&
            dpb_ctx->i_type != H264E_HAL_SLICE_TYPE_B ) {
            RK_S32 diff = dpb_ctx->i_frame_num - dpb_ctx->frames.reference[ref]->i_frame_num;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_difference_of_pic_nums = diff;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_poc = dpb_ctx->frames.reference[ref]->i_poc;
            h264e_rkv_frame_push_unused( dpb_ctx, h264e_rkv_frame_shift( &dpb_ctx->frames.reference[ref] ) );
            dpb_ctx->b_ref_reorder[0] = 1;
            ref--;
        }
    }
}
#endif

static RK_S32 h264e_rkv_reference_distance( H264eRefParam *ref_cfg, H264eRkvDpbCtx *dpb_ctx, H264eRkvFrame *frame )
{
    if ( ref_cfg->i_frame_packing == 5 )
        return abs((dpb_ctx->fdec->i_frame_cnt & ~1) - (frame->i_frame_cnt & ~1)) +
               ((dpb_ctx->fdec->i_frame_cnt & 1) != (frame->i_frame_cnt & 1));
    else
        return abs(dpb_ctx->fdec->i_frame_cnt - frame->i_frame_cnt);
}


static void h264e_rkv_reference_build_list(H264eHalContext *ctx)
{
    RK_S32 b_ok = 1, i = 0, list = 0, j = 0;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eHalParam *par = &ctx->param;
    H264eRefParam *ref_cfg = &par->ref;
    H264eRkvFrame *fdec = dpb_ctx->fdec;
    RK_S32 i_poc = fdec->i_poc;

    h264e_hal_enter();
    /* build ref list 0/1 */
    dpb_ctx->i_ref[0] = 0;
    dpb_ctx->i_ref[1] = 0;

    if ( dpb_ctx->i_slice_type == H264E_HAL_SLICE_TYPE_I ) {
        if ( ref_cfg->i_long_term_en && ref_cfg->i_frame_reference > 1 )
            ref_cfg->hw_longterm_mode ^= 1;       //0 and 1, circle; If ref==1 , longterm mode only 1;

        if ( ref_cfg->i_long_term_en && ref_cfg->hw_longterm_mode && ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0))
            fdec->long_term_flag = 1;
        h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->i_slice_type == SLICE_TYPE_I, return");
        return;
    }

    h264e_hal_dbg(H264E_DBG_DPB, "fdec->i_poc: %d", fdec->i_poc);
    for ( i = 0; dpb_ctx->frames.reference[i]; i++ ) {
        if ( dpb_ctx->frames.reference[i]->b_corrupt )
            continue;
        if ( dpb_ctx->frames.reference[i]->i_poc < i_poc )
            dpb_ctx->fref[0][dpb_ctx->i_ref[0]++] = dpb_ctx->frames.reference[i];
        else if ( dpb_ctx->frames.reference[i]->i_poc > i_poc )
            dpb_ctx->fref[1][dpb_ctx->i_ref[1]++] = dpb_ctx->frames.reference[i];
    }

    h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->i_ref[0]: %d", dpb_ctx->i_ref[0]);
    h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->i_ref[1]: %d", dpb_ctx->i_ref[1]);

    if ( dpb_ctx->i_mmco_remove_from_end ) {
        /* Order ref0 for MMCO remove */
        do {
            b_ok = 1;
            for (i = 0; i < dpb_ctx->i_ref[0] - 1; i++ ) {
                if ( dpb_ctx->fref[0][i]->i_frame_cnt < dpb_ctx->fref[0][i + 1]->i_frame_cnt ) {
                    MPP_SWAP( H264eRkvFrame *, dpb_ctx->fref[0][i], dpb_ctx->fref[0][i + 1] );
                    b_ok = 0;
                    break;
                }
            }
        } while ( !b_ok );

        for ( i = dpb_ctx->i_ref[0] - 1; i >= dpb_ctx->i_ref[0] - dpb_ctx->i_mmco_remove_from_end; i-- ) {
            RK_S32 diff = dpb_ctx->fdec->i_frame_num - dpb_ctx->fref[0][i]->i_frame_num;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][i]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = diff;
        }
    }

    /* Order reference lists by distance from the current frame. */
    for ( list = 0; list < 1; list++ ) {    //only one list
        dpb_ctx->fref_nearest[list] = dpb_ctx->fref[list][0];
        do {
            b_ok = 1;
            for ( i = 0; i < dpb_ctx->i_ref[list] - 1; i++ ) {
                if ( list ? dpb_ctx->fref[list][i + 1]->i_poc < dpb_ctx->fref_nearest[list]->i_poc
                     : dpb_ctx->fref[list][i + 1]->i_poc > dpb_ctx->fref_nearest[list]->i_poc )
                    dpb_ctx->fref_nearest[list] = dpb_ctx->fref[list][i + 1];
                if ( h264e_rkv_reference_distance( ref_cfg, dpb_ctx, dpb_ctx->fref[list][i]   ) >
                     h264e_rkv_reference_distance( ref_cfg, dpb_ctx, dpb_ctx->fref[list][i + 1] ) ) {
                    MPP_SWAP( H264eRkvFrame *, dpb_ctx->fref[list][i], dpb_ctx->fref[list][i + 1] );
                    b_ok = 0;
                    break;
                }
            }
        } while ( !b_ok );
    }
    //Order long_term frame to last lists  , only one long_term frame
    if (ref_cfg->i_long_term_en) {
        for (i = 0; i < dpb_ctx->i_ref[0]; i++) {
            if (dpb_ctx->fref[0][i]->long_term_flag == 1) {
                for (j = i; j < dpb_ctx->i_ref[0] - 1; j++) {
                    MPP_SWAP(H264eRkvFrame *, dpb_ctx->fref[0][j], dpb_ctx->fref[0][j + 1]);
                }
                break;
            }
        }
    }

    //reorder, when in longterm_mode "1", don't reorder;
    if (ref_cfg->i_frame_reference > 1 && ref_cfg->i_ref_pos && ref_cfg->i_ref_pos < dpb_ctx->i_ref[0])
        dpb_ctx->b_ref_pic_list_reordering[0] = 1;
    else
        dpb_ctx->b_ref_pic_list_reordering[0] = 0;
    if (dpb_ctx->b_ref_pic_list_reordering[0]) {
        for (list = 0; list < 1; list++) {
            if (dpb_ctx->fref[0][ref_cfg->i_ref_pos]->long_term_flag) {
                fdec->reorder_longterm_flag = 1;
            }
            for (i = ref_cfg->i_ref_pos; i >= 1; i--) {
                MPP_SWAP(H264eRkvFrame *, dpb_ctx->fref[list][i], dpb_ctx->fref[list][i - 1]);
            }
        }
    }

    //first P frame mark max_long_term_frame_idx_plus1
    if ( ref_cfg->i_long_term_en && dpb_ctx->fdec->i_frame_num == 1 ) {
        dpb_ctx->i_mmco_command_count = 0;

        dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = 0;
        dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 4;
        dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = 2;  //  max_long_term_frame_idx_plus1 , slice will plus 1, is 0;
    } else
        dpb_ctx->i_mmco_command_count = 0;

    //longterm marking
    if ( ref_cfg->i_long_term_en && ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0)) {
        RK_S32 reflist_longterm = 0;
        RK_S32 reflist_longidx  = 0;
        RK_S32 reflist_short_to_long  = 0;
        RK_S32 reflist_short_to_long_idx  = 0;

        //search frame for transferring short to long;
        if (ref_cfg->hw_longterm_mode == 0 || dpb_ctx->fdec->i_frame_num == 1) {
            for (i = 0; i < dpb_ctx->i_ref[0]; i++) {
                if (!dpb_ctx->fref[0][i]->long_term_flag && (ref_cfg->i_ref_pos + 1) == i) {
                    reflist_short_to_long++;
                    reflist_short_to_long_idx = i;
                    break;
                }
            }
            //clear ref longterm flag
            for (i = 0; i < dpb_ctx->i_ref[0]; i++) {
                if (dpb_ctx->fref[0][i]->long_term_flag) {
                    reflist_longterm++;
                    reflist_longidx = i;
                    dpb_ctx->fref[0][i]->long_term_flag = 0;
                }
            }
        }

        mpp_assert(reflist_longterm <= 1);

        //marking ref longterm to unused;
        if ( reflist_longterm == 1 ) {
            i = reflist_longidx;

            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][i]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 2;     //long_term_pic_num
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = 1;
        } else if ( dpb_ctx->i_ref[0] >= sps->i_num_ref_frames && dpb_ctx->i_ref[0] > 1 ) { //if dpb is full, so it need release a short term ref frame;
            //if longterm marking is same with release short term, change release short term;
            RK_S32 pos = ((reflist_short_to_long && reflist_short_to_long_idx == (dpb_ctx->i_ref[0] - 1)) || dpb_ctx->fref[0][dpb_ctx->i_ref[0] - 1]->long_term_flag) + 1;
            RK_S32 diff = dpb_ctx->fdec->i_frame_num - dpb_ctx->fref[0][dpb_ctx->i_ref[0] - pos]->i_frame_num;

            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][dpb_ctx->i_ref[0] - pos]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 1;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = diff;  // difference_of_pic_nums_minus1
        }

        //marking curr pic to longterm;
        if ( ref_cfg->hw_longterm_mode && dpb_ctx->fdec->i_frame_num == 1) {
            fdec->long_term_flag = 1;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = 0;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 6;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = 1;  //  long_term_frame_idx ;
        } else if ( reflist_short_to_long ) { //Assign a long-term frame index to a short-term picture
            i = reflist_short_to_long_idx;
            RK_S32 diff = dpb_ctx->fdec->i_frame_num - dpb_ctx->fref[0][i]->i_frame_num;

            dpb_ctx->fref[0][i]->long_term_flag = 1;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][i]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 3; //short term to long term;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = diff;  //  difference_of_pic_nums_minus1 , slice will minus 1, is 0;
        }
    } else {
        if (!(ref_cfg->i_long_term_en && dpb_ctx->fdec->i_frame_num == 1))
            dpb_ctx->i_mmco_command_count = 0;
    }

    dpb_ctx->i_ref[1] = H264E_HAL_MIN( dpb_ctx->i_ref[1], dpb_ctx->i_max_ref1 );
    dpb_ctx->i_ref[0] = H264E_HAL_MIN( dpb_ctx->i_ref[0], dpb_ctx->i_max_ref0 );
    dpb_ctx->i_ref[0] = H264E_HAL_MIN( dpb_ctx->i_ref[0], ref_cfg->i_frame_reference ); // if reconfig() has lowered the limit

    /* EXP: add duplicates */
    mpp_assert( dpb_ctx->i_ref[0] + dpb_ctx->i_ref[1] <= H264E_REF_MAX );

    h264e_hal_leave();
}

static MPP_RET h264e_rkv_reference_update( H264eHalContext *ctx)
{
    RK_S32 i = 0, j = 0;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRefParam *ref_cfg = &ctx->param.ref;

    h264e_hal_enter();
    if ( !dpb_ctx->fdec->b_kept_as_ref ) {
        h264e_hal_err("!dpb_ctx->fdec->b_kept_as_ref, return early");
        return MPP_OK;
    }

    /* apply mmco from previous frame. */
    for ( i = 0; i < dpb_ctx->i_mmco_command_count; i++ ) {
        RK_S32 mmco = dpb_ctx->mmco[i].memory_management_control_operation;
        for ( j = 0; dpb_ctx->frames.reference[j]; j++ ) {
            if ( dpb_ctx->frames.reference[j]->i_poc == dpb_ctx->mmco[i].i_poc && (mmco == 1 || mmco == 2) )
                h264e_rkv_frame_push_unused( dpb_ctx, h264e_rkv_frame_shift( &dpb_ctx->frames.reference[j] ) );
        }
    }

    /* move frame in the buffer */
    h264e_hal_dbg(H264E_DBG_DPB, "try to push dpb_ctx->fdec %p, poc %d", dpb_ctx->fdec, dpb_ctx->fdec->i_poc);
    h264e_rkv_frame_push( dpb_ctx->frames.reference, dpb_ctx->fdec );
    if ( ref_cfg->i_long_term_en ) {
        if ( dpb_ctx->frames.reference[sps->i_num_ref_frames] ) {
            for (i = 0; i < 17; i++) {
                if (dpb_ctx->frames.reference[i]->long_term_flag == 0) { //if longterm , don't remove;
                    h264e_rkv_frame_push_unused(dpb_ctx, h264e_rkv_frame_shift(&dpb_ctx->frames.reference[i]));
                    break;
                }
                mpp_assert(i != 16);
            }
        }
    } else {
        if ( dpb_ctx->frames.reference[sps->i_num_ref_frames] )
            h264e_rkv_frame_push_unused( dpb_ctx, h264e_rkv_frame_shift( dpb_ctx->frames.reference ) );
    }

    h264e_hal_leave();
    return MPP_OK;
}



static MPP_RET h264e_rkv_reference_frame_set( H264eHalContext *ctx, H264eHwCfg *syn)
{
    RK_U32 i_nal_type = 0, i_nal_ref_idc = 0;
    RK_S32 list = 0, k = 0;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264eRefParam *ref_cfg = &ctx->param.ref;

    h264e_hal_enter();

    dpb_ctx->fdec = h264e_rkv_frame_pop_unused(ctx);
    if ( !dpb_ctx->fdec ) {
        h264e_hal_err("!dpb_ctx->fdec, current recon buf get failed");
        return MPP_NOK;
    }

    dpb_ctx->i_max_ref0 = ref_cfg->i_frame_reference;
    dpb_ctx->i_max_ref1 = H264E_HAL_MIN( sps->vui.i_num_reorder_frames, ref_cfg->i_frame_reference );

    if (syn->coding_type == RKVENC_CODING_TYPE_IDR) {
        dpb_ctx->i_frame_num = 0;
        dpb_ctx->frames.i_last_idr = dpb_ctx->i_frame_cnt;
    }

    dpb_ctx->fdec->i_frame_cnt = dpb_ctx->i_frame_cnt;
    dpb_ctx->fdec->i_frame_num = dpb_ctx->i_frame_num;
    dpb_ctx->fdec->i_frame_type = syn->coding_type;
    dpb_ctx->fdec->i_poc = 2 * ( dpb_ctx->fdec->i_frame_cnt - H264E_HAL_MAX( dpb_ctx->frames.i_last_idr, 0 ) );


    if ( !RKVENC_IS_TYPE_I( dpb_ctx->fdec->i_frame_type ) ) {
        RK_S32 valid_refs_left = 0;
        for ( k = 0; dpb_ctx->frames.reference[k]; k++ )
            if ( !dpb_ctx->frames.reference[k]->b_corrupt )
                valid_refs_left++;
        /* No valid reference frames left: force an IDR. */
        if ( !valid_refs_left ) {
            dpb_ctx->fdec->b_keyframe = 1;
            dpb_ctx->fdec->i_frame_type = RKVENC_CODING_TYPE_IDR;
        }
    }
    if ( dpb_ctx->fdec->b_keyframe )
        dpb_ctx->frames.i_last_keyframe = dpb_ctx->fdec->i_frame_cnt;


    dpb_ctx->i_mmco_command_count =
        dpb_ctx->i_mmco_remove_from_end = 0;
    dpb_ctx->b_ref_pic_list_reordering[0] = 0;
    dpb_ctx->b_ref_pic_list_reordering[1] = 0;

    /* calculate nal type and nal ref idc */
    if (syn->coding_type == RKVENC_CODING_TYPE_IDR) { //TODO: extend syn->frame_coding_type definition
        /* reset ref pictures */
        i_nal_type    = H264E_NAL_SLICE_IDR;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGHEST;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_I;
        h264e_rkv_reference_reset(dpb_ctx);
    } else if ( syn->coding_type == RKVENC_CODING_TYPE_I ) {
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_I;
    } else if ( syn->coding_type == RKVENC_CODING_TYPE_P ) {
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_P;
    } else if ( syn->coding_type == RKVENC_CODING_TYPE_BREF ) {
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_B;
    } else { /* B frame */
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_DISPOSABLE;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_B;
    }

    dpb_ctx->fdec->b_kept_as_ref = i_nal_ref_idc != H264E_NAL_PRIORITY_DISPOSABLE;// && h->param.i_keyint_max > 1;


    if (sps->keyframe_max_interval == 1)
        i_nal_ref_idc = H264E_NAL_PRIORITY_LOW;

    dpb_ctx->i_nal_ref_idc = i_nal_ref_idc;
    dpb_ctx->i_nal_type = i_nal_type;


    dpb_ctx->fdec->i_pts = dpb_ctx->fdec->i_pts;


    /* build list */
    h264e_rkv_reference_build_list(ctx);

    /* set syntax (slice header) */

    /* If the ref list isn't in the default order, construct reordering header */
    for (list = 0; list < 2; list++ ) {
        if ( dpb_ctx->b_ref_pic_list_reordering[list] ) {
            RK_S32 pred_frame_num = dpb_ctx->fdec->i_frame_num & ((1 << sps->i_log2_max_frame_num) - 1);
            for ( k = 0; k < dpb_ctx->i_ref[list]; k++ ) {
                if ( dpb_ctx->fdec->reorder_longterm_flag ) { //
                    dpb_ctx->fdec->reorder_longterm_flag = 0;     //clear reorder_longterm_flag
                    dpb_ctx->ref_pic_list_order[list][k].idc = 2; //reorder long_term_pic_num;
                    dpb_ctx->ref_pic_list_order[list][k].arg = 0; //long_term_pic_num
                    break;      //NOTE: RK feature: only reorder one time
                } else {
                    RK_S32 lx_frame_num = dpb_ctx->fref[list][k]->i_frame_num  & ((1 << sps->i_log2_max_frame_num) - 1);
                    RK_S32 diff = lx_frame_num - pred_frame_num;
                    dpb_ctx->ref_pic_list_order[list][k].idc = ( diff > 0 );
                    dpb_ctx->ref_pic_list_order[list][k].arg = (abs(diff) - 1) & ((1 << sps->i_log2_max_frame_num) - 1);
                    pred_frame_num = dpb_ctx->fref[list][k]->i_frame_num;
                    break;      //NOTE: RK feature: only reorder one time
                }
            }
        }
    }

    if (dpb_ctx->i_nal_type == H264E_NAL_SLICE_IDR) {
        if (ref_cfg->i_long_term_en && ref_cfg->hw_longterm_mode && ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0) )
            dpb_ctx->i_long_term_reference_flag = 1;
        dpb_ctx->i_idr_pic_id = dpb_ctx->i_tmp_idr_pic_id;
        dpb_ctx->i_tmp_idr_pic_id ^= 1;
    } else {
        dpb_ctx->i_long_term_reference_flag = 0;
        dpb_ctx->i_idr_pic_id = -1;
    }


    h264e_hal_leave();

    return MPP_OK;
}


static MPP_RET h264e_rkv_reference_frame_update( H264eHalContext *ctx)
{
    //H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;

    h264e_hal_enter();
    if (MPP_OK != h264e_rkv_reference_update(ctx)) {
        h264e_hal_err("reference update failed, return now");
        return MPP_NOK;
    }

    h264e_hal_leave();
    return MPP_OK;
}

static MPP_RET h264e_rkv_free_buffers(H264eHalContext *ctx)
{
    RK_S32 k = 0;
    h264e_hal_rkv_buffers *buffers = (h264e_hal_rkv_buffers *)ctx->buffers;
    h264e_hal_enter();
    for (k = 0; k < 2; k++) {
        if (buffers->hw_pp_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_pp_buf[k])) {
                h264e_hal_err("hw_pp_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    for (k = 0; k < 2; k++) {
        if (buffers->hw_dsp_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_dsp_buf[k])) {
                h264e_hal_err("hw_dsp_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_mei_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_mei_buf[k])) {
                h264e_hal_err("hw_mei_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_roi_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_roi_buf[k])) {
                h264e_hal_err("hw_roi_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    {
        RK_S32 num_buf = MPP_ARRAY_ELEMS(buffers->hw_rec_buf);
        for (k = 0; k < num_buf; k++) {
            if (buffers->hw_rec_buf[k]) {
                if (MPP_OK != mpp_buffer_put(buffers->hw_rec_buf[k])) {
                    h264e_hal_err("hw_rec_buf[%d] put failed", k);
                    return MPP_NOK;
                }
            }
        }
    }



    for (k = 0; k < H264E_HAL_RKV_BUF_GRP_BUTT; k++) {
        if (buffers->hw_buf_grp[k]) {
            if (MPP_OK != mpp_buffer_group_put(buffers->hw_buf_grp[k])) {
                h264e_hal_err("buf group[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_rkv_allocate_buffers(H264eHalContext *ctx, H264eHwCfg *syn)
{
    RK_S32 k = 0;
    h264e_hal_rkv_buffers *buffers = (h264e_hal_rkv_buffers *)ctx->buffers;
    RK_U32 num_mbs_oneframe = (syn->width + 15) / 16 * ((syn->height + 15) / 16);
    RK_U32 frame_size = ((syn->width + 15) & (~15)) * ((syn->height + 15) & (~15)); //only Y component
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRkvFrame *frame_buf = dpb_ctx->frame_buf;
    h264e_hal_enter();

    switch ((H264eRkvCsp)syn->input_format) {
    case H264E_RKV_CSP_YUV420P:
    case H264E_RKV_CSP_YUV420SP: {
        frame_size = frame_size * 3 / 2;
        break;
    }
    case H264E_RKV_CSP_YUV422P:
    case H264E_RKV_CSP_YUV422SP:
    case H264E_RKV_CSP_YUYV422:
    case H264E_RKV_CSP_UYVY422:
    case H264E_RKV_CSP_BGR565: {
        frame_size *= 2;
        break;
    }
    case H264E_RKV_CSP_BGR888: {
        frame_size *= 3;
        break;
    }
    case H264E_RKV_CSP_BGRA8888: {
        frame_size *= 4;
        break;
    }
    default: {
        h264e_hal_err("unvalid src color space: %d, return early", syn->input_format);
        return MPP_NOK;
    }
    }

    for (k = 0; k < H264E_HAL_RKV_BUF_GRP_BUTT; k++) {
        if (MPP_OK != mpp_buffer_group_get_internal(&buffers->hw_buf_grp[k], MPP_BUFFER_TYPE_ION)) {
            h264e_hal_err("buf group[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }

    if (syn->preproc_en) {
        for (k = 0; k < 2; k++) {
            if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_PP], &buffers->hw_pp_buf[k], frame_size)) {
                h264e_hal_err("hw_pp_buf[%d] get failed", k);
                return MPP_ERR_MALLOC;
            } else {
                h264e_hal_dbg(H264E_DBG_DPB, "hw_pp_buf[%d] %p done, fd %d", k, buffers->hw_pp_buf[k], mpp_buffer_get_fd(buffers->hw_pp_buf[k]));
            }
        }
    }

    for (k = 0; k < 2; k++) {
        if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_DSP], &buffers->hw_dsp_buf[k], frame_size / 16)) {
            h264e_hal_err("hw_dsp_buf[%d] get failed", k);
            return MPP_ERR_MALLOC;
        } else {
            h264e_hal_dbg(H264E_DBG_DPB, "hw_dsp_buf[%d] %p done, fd %d", k, buffers->hw_dsp_buf[k], mpp_buffer_get_fd(buffers->hw_dsp_buf[k]));
        }
    }

#if 0 //default setting
    RK_U32 num_mei_oneframe = (syn->width + 255) / 256 * ((syn->height + 15) / 16);
    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_MEI], &buffers->hw_mei_buf[k], num_mei_oneframe * 16 * 4)) {
            h264e_hal_err("hw_mei_buf[%d] get failed", k);
            return MPP_ERR_MALLOC;
        } else {
            h264e_hal_dbg(H264E_DBG_DPB, "hw_mei_buf[%d] %p done, fd %d", k, buffers->hw_mei_buf[k], mpp_buffer_get_fd(buffers->hw_mei_buf[k]));
        }
    }
#endif

    if (syn->roi_en) {
        for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
            if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_ROI], &buffers->hw_roi_buf[k], num_mbs_oneframe * 1)) {
                h264e_hal_err("hw_roi_buf[%d] get failed", k);
                return MPP_ERR_MALLOC;
            } else {
                h264e_hal_dbg(H264E_DBG_DPB, "hw_roi_buf[%d] %p done, fd %d", k, buffers->hw_roi_buf[k], mpp_buffer_get_fd(buffers->hw_roi_buf[k]));
            }
        }
    }

    {
        RK_S32 num_buf = MPP_ARRAY_ELEMS(buffers->hw_rec_buf);
        for (k = 0; k < num_buf; k++) {
            if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_REC], &buffers->hw_rec_buf[k], frame_size)) {
                h264e_hal_err("hw_rec_buf[%d] get failed", k);
                return MPP_ERR_MALLOC;
            } else {
                h264e_hal_dbg(H264E_DBG_DPB, "hw_rec_buf[%d] %p done, fd %d", k, buffers->hw_rec_buf[k], mpp_buffer_get_fd(buffers->hw_rec_buf[k]));
            }
            frame_buf[k].hw_buf = buffers->hw_rec_buf[k];
        }
    }

    h264e_hal_leave();
    return MPP_OK;
}

static void h264e_rkv_adjust_param(H264eHalContext *ctx)
{
    H264eHalParam *par = &ctx->param;
    (void)par;
}

static RK_S32 h264e_rkv_stream_get_pos(H264eRkvStream *s)
{
    return (RK_S32)(8 * (s->p - s->p_start) + (4 * 8) - s->i_left);
}

static MPP_RET h264e_rkv_stream_init(H264eRkvStream *s)
{
    RK_S32 offset = 0;
    s->buf = mpp_calloc(RK_U8, H264E_EXTRA_INFO_BUF_SIZE);
    s->buf_plus8 = s->buf + 8; //NOTE: prepare for align

    offset = (size_t)(s->buf_plus8) & 3;
    s->p = s->p_start = s->buf_plus8 - offset;
    //s->p_end = (RK_U8*)s->buf + i_data;
    s->i_left = (4 - offset) * 8;
    //s->cur_bits = endian_fix32(M32(s->p));
    s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16)
                  + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
    s->cur_bits >>= (4 - offset) * 8;
    s->count_bit = 0;

    return MPP_OK;
}


static MPP_RET h264e_rkv_stream_reset(H264eRkvStream *s)
{
    RK_S32 offset = 0;
    h264e_hal_enter();

    offset = (size_t)(s->buf_plus8) & 3;
    s->p = s->p_start;
    s->i_left = (4 - offset) * 8;
    s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16)
                  + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
    s->cur_bits >>= (4 - offset) * 8;
    s->count_bit = 0;

    h264e_hal_leave();
    return MPP_OK;
}

static MPP_RET h264e_rkv_stream_deinit(H264eRkvStream *s)
{
    MPP_FREE(s->buf);

    s->p = NULL;
    s->p_start = NULL;
    //s->p_end = NULL;

    s->i_left = 0;
    s->cur_bits = 0;
    s->count_bit = 0;

    return MPP_OK;
}

static MPP_RET h264e_rkv_stream_realign(H264eRkvStream *s)
{
    RK_S32 offset = (size_t)(s->p) & 3; //used to judge alignment
    if (offset) {
        s->p = s->p - offset; //move pointer to 32bit aligned pos
        s->i_left = (4 - offset) * 8; //init
        //s->cur_bits = endian_fix32(M32(s->p));
        s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16)
                      + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
        s->cur_bits >>= (4 - offset) * 8; //shift right the invalid bit
    }

    return MPP_OK;
}

static MPP_RET
h264e_rkv_stream_write_with_log(H264eRkvStream *s, RK_S32 i_count,
                                RK_U32 val, char *name)
{
    RK_U32 i_bits = val;

    h264e_hal_dbg(H264E_DBG_HEADER, "write bits name %s, count %d, val %d",
                  name, i_count, val);

    s->count_bit += i_count;
    if (i_count < s->i_left) {
        s->cur_bits = (s->cur_bits << i_count) | i_bits;
        s->i_left -= i_count;
    } else {
        i_count -= s->i_left;
        s->cur_bits = (s->cur_bits << s->i_left) | (i_bits >> i_count);
        //M32(s->p) = endian_fix32(s->cur_bits);
        *(s->p) = 0;
        *(s->p) = (s->cur_bits >> 24) & 0xff;
        *(s->p + 1) = (s->cur_bits >> 16) & 0xff;
        *(s->p + 2) = (s->cur_bits >> 8) & 0xff;
        *(s->p + 3) = (s->cur_bits >> 0) & 0xff;
        s->p += 4;
        s->cur_bits = i_bits;
        s->i_left = 32 - i_count;
    }
    return MPP_OK;
}

static MPP_RET
h264e_rkv_stream_write1_with_log(H264eRkvStream *s, RK_U32 val, char *name)
{
    RK_U32 i_bit = val;

    h264e_hal_dbg(H264E_DBG_HEADER, "write 1 bit name %s, val %d", name, val);

    s->count_bit += 1;
    s->cur_bits <<= 1;
    s->cur_bits |= i_bit;
    s->i_left--;
    if (s->i_left == 4 * 8 - 32) {
        //M32(s->p) = endian_fix32(s->cur_bits);
        *(s->p) = (s->cur_bits >> 24) & 0xff;
        *(s->p + 1) = (s->cur_bits >> 16) & 0xff;
        *(s->p + 2) = (s->cur_bits >> 8) & 0xff;
        *(s->p + 3) = (s->cur_bits >> 0) & 0xff;
        s->p += 4;
        s->i_left = 4 * 8;
    }
    return MPP_OK;
}

static MPP_RET
h264e_rkv_stream_write_ue_with_log(H264eRkvStream *s, RK_U32 val, char *name)
{
    RK_S32 size = 0;
    RK_S32 tmp = ++val;

    h264e_hal_dbg(H264E_DBG_HEADER,
                  "write UE bits name %s, val %d (2 steps below are real writting)",
                  name, val);
    if (tmp >= 0x10000) {
        size = 32;
        tmp >>= 16;
    }
    if (tmp >= 0x100) {
        size += 16;
        tmp >>= 8;
    }
    size += h264e_ue_size_tab[tmp];

    h264e_rkv_stream_write_with_log(s, size >> 1, 0, name);
    h264e_rkv_stream_write_with_log(s, (size >> 1) + 1, val, name);

    return MPP_OK;
}

static MPP_RET
h264e_rkv_stream_write_se_with_log(H264eRkvStream *s, RK_S32 val, char *name)
{
    RK_S32 size = 0;
    RK_S32 tmp = 1 - val * 2;
    if (tmp < 0)
        tmp = val * 2;

    val = tmp;
    if (tmp >= 0x100) {
        size = 16;
        tmp >>= 8;
    }
    size += h264e_ue_size_tab[tmp];

    return h264e_rkv_stream_write_with_log(s, size, val, name);
}

static MPP_RET
h264e_rkv_stream_write32(H264eRkvStream *s, RK_U32 i_bits, char *name)
{
    h264e_rkv_stream_write_with_log(s, 16, i_bits >> 16, name);
    h264e_rkv_stream_write_with_log(s, 16, i_bits      , name);

    return MPP_OK;
}

static RK_S32 h264e_rkv_stream_size_se( RK_S32 val )
{
    RK_S32 tmp = 1 - val * 2;
    if ( tmp < 0 ) tmp = val * 2;
    if ( tmp < 256 )
        return h264e_ue_size_tab[tmp];
    else
        return h264e_ue_size_tab[tmp >> 8] + 16;
}


static MPP_RET h264e_rkv_stream_rbsp_trailing(H264eRkvStream *s)
{
    //align bits, 1+N zeros.
    h264e_rkv_stream_write1_with_log(s, 1, "align_1_bit");
    h264e_rkv_stream_write_with_log(s, s->i_left & 7, 0, "align_N_bits");

    return MPP_OK;
}

/*
 * Write the rest of cur_bits to the bitstream;
 * results in a bitstream no longer 32-bit aligned.
 */
static MPP_RET h264e_rkv_stream_flush(H264eRkvStream *s)
{
    //do 4 bytes aligned
    //M32(s->p) = endian_fix32(s->cur_bits << (s->i_left & 31));
    RK_U32 tmp_bit = s->cur_bits << (s->i_left & 31);
    *(s->p) = (tmp_bit >> 24) & 0xff;
    *(s->p + 1) = (tmp_bit >> 16) & 0xff;
    *(s->p + 2) = (tmp_bit >> 8) & 0xff;
    *(s->p + 3) = (tmp_bit >> 0) & 0xff;
    /*
     * p point to bit which aligned, rather than
     * the pos next to 4-byte alinged
     */
    s->p += 4 - (s->i_left >> 3);
    s->i_left = 4 * 8;

    return MPP_OK;
}

static void h264e_rkv_nals_init(H264eRkvExtraInfo *out)
{
    out->nal_buf = mpp_calloc(RK_U8, H264E_EXTRA_INFO_BUF_SIZE);
    out->nal_num = 0;
}

static void h264e_rkv_nals_deinit(H264eRkvExtraInfo *out)
{
    MPP_FREE(out->nal_buf);

    out->nal_num = 0;
}

static void
h264e_rkv_nal_start(H264eRkvExtraInfo *out, RK_S32 i_type, RK_S32 i_ref_idc)
{
    H264eRkvStream *s = &out->stream;
    H264eRkvNal *nal = &out->nal[out->nal_num];

    nal->i_ref_idc = i_ref_idc;
    nal->i_type = i_type;
    nal->b_long_startcode = 1;

    nal->i_payload = 0;
    /* NOTE: consistent with stream_init */
    nal->p_payload = &s->buf_plus8[h264e_rkv_stream_get_pos(s) / 8];
    nal->i_padding = 0;
}

static void h264e_rkv_nal_end(H264eRkvExtraInfo *out)
{
    H264eRkvNal *nal = &(out->nal[out->nal_num]);
    H264eRkvStream *s = &out->stream;
    /* NOTE: consistent with stream_init */
    RK_U8 *end = &s->buf_plus8[h264e_rkv_stream_get_pos(s) / 8];
    nal->i_payload = (RK_S32)(end - nal->p_payload);
    /*
     * Assembly implementation of nal_escape reads past the end of the input.
     * While undefined padding wouldn't actually affect the output,
     * it makes valgrind unhappy.
     */
    memset(end, 0xff, 64);
    out->nal_num++;
}

static RK_U8 *h264e_rkv_nal_escape_c(RK_U8 *dst, RK_U8 *src, RK_U8 *end)
{
    if (src < end) *dst++ = *src++;
    if (src < end) *dst++ = *src++;
    while (src < end) {
        if (src[0] <= 0x03 && !dst[-2] && !dst[-1])
            *dst++ = 0x03;
        *dst++ = *src++;
    }
    return dst;
}

static void h264e_rkv_nal_encode(RK_U8 *dst, H264eRkvNal *nal)
{
    RK_S32 b_annexb = 1;
    RK_S32 size = 0;
    RK_U8 *src = nal->p_payload;
    RK_U8 *end = nal->p_payload + nal->i_payload;
    RK_U8 *orig_dst = dst;

    if (b_annexb) {
        //if (nal->b_long_startcode)//fix by gsl
        //*dst++ = 0x00;//fix by gsl
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x01;
    } else /* save room for size later */
        dst += 4;

    /* nal header */
    *dst++ = (0x00 << 7) | (nal->i_ref_idc << 5) | nal->i_type;

    dst = h264e_rkv_nal_escape_c(dst, src, end);
    size = (RK_S32)((dst - orig_dst) - 4);

    /* Write the size header for mp4/etc */
    if (!b_annexb) {
        /* Size doesn't include the size of the header we're writing now. */
        orig_dst[0] = size >> 24;
        orig_dst[1] = size >> 16;
        orig_dst[2] = size >> 8;
        orig_dst[3] = size >> 0;
    }

    nal->i_payload = size + 4;
    nal->p_payload = orig_dst;
}

static MPP_RET h264e_rkv_encapsulate_nals(H264eRkvExtraInfo *out)
{
    RK_S32 i = 0;
    RK_S32 i_avcintra_class = 0;
    RK_S32 nal_size = 0;
    RK_S32 necessary_size = 0;
    RK_U8 *nal_buffer = out->nal_buf;
    RK_S32 nal_num = out->nal_num;
    H264eRkvNal *nal = out->nal;

    h264e_hal_enter();

    for (i = 0; i < nal_num; i++)
        nal_size += nal[i].i_payload;

    /* Worst-case NAL unit escaping: reallocate the buffer if it's too small. */
    necessary_size = nal_size * 3 / 2 + nal_num * 4 + 4 + 64;
    for (i = 0; i < nal_num; i++)
        necessary_size += nal[i].i_padding;

    for (i = 0; i < nal_num; i++) {
        nal[i].b_long_startcode = !i ||
                                  nal[i].i_type == H264E_NAL_SPS ||
                                  nal[i].i_type == H264E_NAL_PPS ||
                                  i_avcintra_class;
        h264e_rkv_nal_encode(nal_buffer, &nal[i]);
        nal_buffer += nal[i].i_payload;
    }

    h264e_hal_dbg(H264E_DBG_HEADER, "nals total size: %d bytes",
                  nal_buffer - out->nal_buf);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET
h264e_rkv_sei_write(H264eRkvStream *s, RK_U8 *payload,
                    RK_S32 payload_size, RK_S32 payload_type)
{
    RK_S32 i = 0;

    h264e_hal_enter();

    s->count_bit = 0;
    h264e_rkv_stream_realign(s);

    for (i = 0; i <= payload_type - 255; i += 255)
        h264e_rkv_stream_write_with_log(s, 8, 0xff,
                                        "sei_payload_type_ff_byte");
    h264e_rkv_stream_write_with_log(s, 8, payload_type - i,
                                    "sei_last_payload_type_byte");

    for (i = 0; i <= payload_size - 255; i += 255)
        h264e_rkv_stream_write_with_log(s, 8, 0xff,
                                        "sei_payload_size_ff_byte");
    h264e_rkv_stream_write_with_log( s, 8, payload_size - i,
                                     "sei_last_payload_size_byte");

    for (i = 0; i < payload_size; i++)
        h264e_rkv_stream_write_with_log(s, 8, (RK_U32)payload[i],
                                        "sei_payload_data");

    h264e_rkv_stream_rbsp_trailing(s);
    h264e_rkv_stream_flush(s);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_rkv_sei_encode(H264eHalContext *ctx)
{
    H264eRkvExtraInfo *info = (H264eRkvExtraInfo *)ctx->extra_info;
    char *str = (char *)info->sei_buf;
    RK_S32 str_len = 0;

    h264e_sei_pack2str(str + H264E_UUID_LENGTH, ctx);

    str_len = strlen(str) + 1;
    if (str_len > H264E_SEI_BUF_SIZE) {
        h264e_hal_err("SEI actual string length %d exceed malloced size %d",
                      str_len, H264E_SEI_BUF_SIZE);
        return MPP_NOK;
    } else {
        h264e_rkv_sei_write(&info->stream, (RK_U8 *)str, str_len, H264E_SEI_USER_DATA_UNREGISTERED);
    }


    return MPP_OK;
}

static MPP_RET h264e_rkv_sps_write(H264eSps *sps, H264eRkvStream *s)
{
    h264e_hal_enter();

    s->count_bit = 0;
    h264e_rkv_stream_realign(s);
    h264e_rkv_stream_write_with_log(s, 8, sps->i_profile_idc, "profile_idc");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set0, "constraint_set0_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set1, "constraint_set1_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set2, "constraint_set2_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set3, "constraint_set3_flag");

    h264e_rkv_stream_write_with_log(s, 4, 0, "reserved_zero_4bits");

    h264e_rkv_stream_write_with_log(s, 8, sps->i_level_idc, "level_idc");

    h264e_rkv_stream_write_ue_with_log(s, sps->i_id, "seq_parameter_set_id");

    if (sps->i_profile_idc >= H264_PROFILE_HIGH) {
        h264e_rkv_stream_write_ue_with_log(s, sps->i_chroma_format_idc, "chroma_format_idc");
        if (sps->i_chroma_format_idc == H264E_CHROMA_444)
            h264e_rkv_stream_write1_with_log(s, 0, "separate_colour_plane_flag");
        h264e_rkv_stream_write_ue_with_log(s, H264_BIT_DEPTH - 8, "bit_depth_luma_minus8");
        h264e_rkv_stream_write_ue_with_log(s, H264_BIT_DEPTH - 8, "bit_depth_chroma_minus8");
        h264e_rkv_stream_write1_with_log(s, sps->b_qpprime_y_zero_transform_bypass, "qpprime_y_zero_transform_bypass_flag");
        h264e_rkv_stream_write1_with_log(s, 0, "seq_scaling_matrix_present_flag");
    }

    h264e_rkv_stream_write_ue_with_log(s, sps->i_log2_max_frame_num - 4, "log2_max_frame_num_minus4");
    h264e_rkv_stream_write_ue_with_log(s, sps->i_poc_type, "pic_order_cnt_type");
    if (sps->i_poc_type == 0)
        h264e_rkv_stream_write_ue_with_log(s, sps->i_log2_max_poc_lsb - 4, "log2_max_pic_order_cnt_lsb_minus4");
    h264e_rkv_stream_write_ue_with_log(s, sps->i_num_ref_frames, "max_num_ref_frames");
    h264e_rkv_stream_write1_with_log(s, sps->b_gaps_in_frame_num_value_allowed, "gaps_in_frame_num_value_allowed_flag");
    h264e_rkv_stream_write_ue_with_log(s, sps->i_mb_width - 1, "pic_width_in_mbs_minus1");
    h264e_rkv_stream_write_ue_with_log(s, (sps->i_mb_height >> !sps->b_frame_mbs_only) - 1, "pic_height_in_map_units_minus1");
    h264e_rkv_stream_write1_with_log(s, sps->b_frame_mbs_only, "frame_mbs_only_flag");
    if (!sps->b_frame_mbs_only)
        h264e_rkv_stream_write1_with_log(s, sps->b_mb_adaptive_frame_field, "mb_adaptive_frame_field_flag");
    h264e_rkv_stream_write1_with_log(s, sps->b_direct8x8_inference, "direct_8x8_inference_flag");

    h264e_rkv_stream_write1_with_log(s, sps->b_crop, "frame_cropping_flag");
    if (sps->b_crop) {
        RK_S32 h_shift = sps->i_chroma_format_idc == H264E_CHROMA_420 || sps->i_chroma_format_idc == H264E_CHROMA_422;
        RK_S32 v_shift = sps->i_chroma_format_idc == H264E_CHROMA_420;
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_left >> h_shift, "frame_crop_left_offset");
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_right >> h_shift, "frame_crop_right_offset");
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_top >> v_shift, "frame_crop_top_offset");
        h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_bottom >> v_shift, "frame_crop_bottom_offset");
    }

    h264e_rkv_stream_write1_with_log(s, sps->vui.b_vui, "vui_parameters_present_flag");
    if (sps->vui.b_vui) {
        h264e_rkv_stream_write1_with_log(s, sps->vui.b_aspect_ratio_info_present, "aspect_ratio_info_present_flag");
        if (sps->vui.b_aspect_ratio_info_present) {
            RK_S32 i = 0;
            static const struct { RK_U8 w, h, sar; } sar[] = {
                // aspect_ratio_idc = 0 -> unspecified
                { 1, 1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
                { 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
                { 80, 33, 9 }, { 18, 11, 10 }, { 15, 11, 11 }, { 64, 33, 12 },
                { 160, 99, 13 }, { 4, 3, 14 }, { 3, 2, 15 }, { 2, 1, 16 },
                // aspect_ratio_idc = [17..254] -> reserved
                { 0, 0, 255 }
            };
            for (i = 0; sar[i].sar != 255; i++) {
                if (sar[i].w == sps->vui.i_sar_width &&
                    sar[i].h == sps->vui.i_sar_height)
                    break;
            }
            h264e_rkv_stream_write_with_log(s, 8, sar[i].sar, "aspect_ratio_idc");
            if (sar[i].sar == 255) { /* aspect_ratio_idc (extended) */
                h264e_rkv_stream_write_with_log(s, 16, sps->vui.i_sar_width, "sar_width");
                h264e_rkv_stream_write_with_log(s, 16, sps->vui.i_sar_height, "sar_height");
            }
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_overscan_info_present, "overscan_info_present_flag");
        if (sps->vui.b_overscan_info_present)
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_overscan_info, "overscan_appropriate_flag");

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_signal_type_present, "video_signal_type_present_flag");
        if (sps->vui.b_signal_type_present) {
            h264e_rkv_stream_write_with_log(s, 3, sps->vui.i_vidformat, "video_format");
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_fullrange, "video_full_range_flag");
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_color_description_present, "colour_description_present_flag");
            if (sps->vui.b_color_description_present) {
                h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_colorprim, "colour_primaries");
                h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_transfer, "transfer_characteristics");
                h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_colmatrix, "matrix_coefficients");
            }
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_chroma_loc_info_present, "chroma_loc_info_present_flag");
        if (sps->vui.b_chroma_loc_info_present) {
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_chroma_loc_top, "chroma_loc_info_present_flag");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_chroma_loc_bottom, "chroma_sample_loc_type_bottom_field");
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_timing_info_present, "chroma_sample_loc_type_bottom_field");
        if (sps->vui.b_timing_info_present) {
            h264e_rkv_stream_write32(s, sps->vui.i_num_units_in_tick, "num_units_in_tick");
            h264e_rkv_stream_write32(s, sps->vui.i_time_scale, "time_scale");
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_fixed_frame_rate, "fixed_frame_rate_flag");
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_nal_hrd_parameters_present, "time_scale");
        if (sps->vui.b_nal_hrd_parameters_present) {
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_cpb_cnt - 1, "cpb_cnt_minus1");
            h264e_rkv_stream_write_with_log(s, 4, sps->vui.hrd.i_bit_rate_scale, "bit_rate_scale");
            h264e_rkv_stream_write_with_log(s, 4, sps->vui.hrd.i_cpb_size_scale, "cpb_size_scale");

            h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_bit_rate_value - 1, "bit_rate_value_minus1");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_cpb_size_value - 1, "cpb_size_value_minus1");

            h264e_rkv_stream_write1_with_log(s, sps->vui.hrd.b_cbr_hrd, "cbr_flag");

            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_initial_cpb_removal_delay_length - 1, "initial_cpb_removal_delay_length_minus1");
            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_cpb_removal_delay_length - 1, "cpb_removal_delay_length_minus1");
            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_dpb_output_delay_length - 1, "dpb_output_delay_length_minus1");
            h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_time_offset_length, "time_offset_length");
        }

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_vcl_hrd_parameters_present, "vcl_hrd_parameters_present_flag");

        if (sps->vui.b_nal_hrd_parameters_present || sps->vui.b_vcl_hrd_parameters_present)
            h264e_rkv_stream_write1_with_log(s, 0, "low_delay_hrd_flag");   /* low_delay_hrd_flag */

        h264e_rkv_stream_write1_with_log(s, sps->vui.b_pic_struct_present, "pic_struct_present_flag");
        h264e_rkv_stream_write1_with_log(s, sps->vui.b_bitstream_restriction, "bitstream_restriction_flag");
        if (sps->vui.b_bitstream_restriction) {
            h264e_rkv_stream_write1_with_log(s, sps->vui.b_motion_vectors_over_pic_boundaries, "motion_vectors_over_pic_boundaries_flag");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_bytes_per_pic_denom, "max_bytes_per_pic_denom");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_bits_per_mb_denom, "max_bits_per_mb_denom");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_log2_max_mv_length_horizontal, "log2_max_mv_length_horizontal");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_log2_max_mv_length_vertical, "log2_max_mv_length_vertical");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_num_reorder_frames, "max_num_reorder_frames");
            h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_dec_frame_buffering, "max_dec_frame_buffering");
        }
    }
    h264e_rkv_stream_rbsp_trailing(s);
    h264e_rkv_stream_flush(s);

    h264e_hal_dbg(H264E_DBG_HEADER, "write pure sps head size: %d bits", s->count_bit);

    h264e_hal_leave();

    return MPP_OK;
}

static void h264e_rkv_scaling_list_write( H264eRkvStream *s, H264ePps *pps, RK_S32 idx )
{
    RK_S32 k = 0;
    const RK_S32 len = idx < 4 ? 16 : 64;
    const RK_U8 *zigzag = idx < 4 ? h264e_zigzag_scan4[0] : h264e_zigzag_scan8[0];
    const RK_U8 *list = pps->scaling_list[idx];
    const RK_U8 *def_list = (idx == H264E_CQM_4IC) ? pps->scaling_list[H264E_CQM_4IY]
                            : (idx == H264E_CQM_4PC) ? pps->scaling_list[H264E_CQM_4PY]
                            : (idx == H264E_CQM_8IC + 4) ? pps->scaling_list[H264E_CQM_8IY + 4]
                            : (idx == H264E_CQM_8PC + 4) ? pps->scaling_list[H264E_CQM_8PY + 4]
                            : h264e_cqm_jvt[idx];
    if ( !memcmp( list, def_list, len ) )
        h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_present_flag");   // scaling_list_present_flag
    else if ( !memcmp( list, h264e_cqm_jvt[idx], len ) ) {
        h264e_rkv_stream_write1_with_log( s, 1, "scaling_list_present_flag");   // scaling_list_present_flag
        h264e_rkv_stream_write_se_with_log( s, -8, "use_jvt_list"); // use jvt list
    } else {
        RK_S32 run;
        h264e_rkv_stream_write1_with_log( s, 1, "scaling_list_present_flag");   // scaling_list_present_flag

        // try run-length compression of trailing values
        for ( run = len; run > 1; run-- )
            if ( list[zigzag[run - 1]] != list[zigzag[run - 2]] )
                break;
        if ( run < len && len - run < h264e_rkv_stream_size_se( (RK_S8) - list[zigzag[run]] ) )
            run = len;

        for ( k = 0; k < run; k++ )
            h264e_rkv_stream_write_se_with_log( s, (RK_S8)(list[zigzag[k]] - (k > 0 ? list[zigzag[k - 1]] : 8)), "delta_scale"); // delta

        if ( run < len )
            h264e_rkv_stream_write_se_with_log( s, (RK_S8) - list[zigzag[run]], "-scale");
    }
}

static MPP_RET h264e_rkv_pps_write(H264ePps *pps, H264eSps *sps, H264eRkvStream *s)
{
    h264e_hal_enter();

    s->count_bit = 0;
    h264e_rkv_stream_realign( s );

    h264e_rkv_stream_write_ue_with_log( s, pps->i_id, "pic_parameter_set_id");
    h264e_rkv_stream_write_ue_with_log( s, pps->i_sps_id, "seq_parameter_set_id");

    h264e_rkv_stream_write1_with_log( s, pps->b_cabac, "entropy_coding_mode_flag");
    h264e_rkv_stream_write1_with_log( s, pps->b_pic_order, "bottom_field_pic_order_in_frame_present_flag");
    h264e_rkv_stream_write_ue_with_log( s, pps->i_num_slice_groups - 1, "num_slice_groups_minus1");

    h264e_rkv_stream_write_ue_with_log( s, pps->i_num_ref_idx_l0_default_active - 1, "num_ref_idx_l0_default_active_minus1");
    h264e_rkv_stream_write_ue_with_log( s, pps->i_num_ref_idx_l1_default_active - 1, "num_ref_idx_l1_default_active_minus1");
    h264e_rkv_stream_write1_with_log( s, pps->b_weighted_pred, "weighted_pred_flag");
    h264e_rkv_stream_write_with_log( s, 2, pps->i_weighted_bipred_idc, "weighted_bipred_idc");

    h264e_rkv_stream_write_se_with_log( s, pps->i_pic_init_qp - 26 - H264_QP_BD_OFFSET, "pic_init_qp_minus26");
    h264e_rkv_stream_write_se_with_log( s, pps->i_pic_init_qs - 26 - H264_QP_BD_OFFSET, "pic_init_qs_minus26");
    h264e_rkv_stream_write_se_with_log( s, pps->i_chroma_qp_index_offset, "chroma_qp_index_offset");

    h264e_rkv_stream_write1_with_log( s, pps->b_deblocking_filter_control, "deblocking_filter_control_present_flag");
    h264e_rkv_stream_write1_with_log( s, pps->b_constrained_intra_pred, "constrained_intra_pred_flag");
    h264e_rkv_stream_write1_with_log( s, pps->b_redundant_pic_cnt, "redundant_pic_cnt_present_flag");

    if ( pps->b_transform_8x8_mode || pps->b_cqm_preset != H264E_CQM_FLAT ) {
        h264e_rkv_stream_write1_with_log( s, pps->b_transform_8x8_mode, "transform_8x8_mode_flag");
        h264e_rkv_stream_write1_with_log( s, (pps->b_cqm_preset != H264E_CQM_FLAT), "pic_scaling_matrix_present_flag");
        if ( pps->b_cqm_preset != H264E_CQM_FLAT ) {
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4IY);
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4IC );
            h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag"); // Cr = Cb TODO:replaced with real name
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4PY );
            h264e_rkv_scaling_list_write( s, pps, H264E_CQM_4PC );
            h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag"); // Cr = Cb TODO:replaced with real name
            if ( pps->b_transform_8x8_mode ) {
                if ( sps->i_chroma_format_idc == H264E_CHROMA_444 ) {
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8IY + 4 );
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8IC + 4 );
                    h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag" ); // Cr = Cb TODO:replaced with real name
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8PY + 4 );
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8PC + 4 );
                    h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag" ); // Cr = Cb TODO:replaced with real name
                } else {
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8IY + 4 );
                    h264e_rkv_scaling_list_write( s, pps, H264E_CQM_8PY + 4 );
                }
            }
        }
        h264e_rkv_stream_write_se_with_log( s, pps->i_second_chroma_qp_index_offset, "second_chroma_qp_index_offset");
    }

    h264e_rkv_stream_rbsp_trailing( s );
    h264e_rkv_stream_flush( s );

    h264e_hal_dbg(H264E_DBG_HEADER, "write pure pps size: %d bits", s->count_bit);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_rkv_init_extra_info(H264eRkvExtraInfo *extra_info)
{
    // random ID number generated according to ISO-11578
    // NOTE: any element of h264e_sei_uuid should NOT be 0x00,
    // otherwise the string length of sei_buf will always be the distance between the
    // element 0x00 address and the sei_buf start address.
    static const RK_U8 h264e_sei_uuid[H264E_UUID_LENGTH] = {
        0x63, 0xfc, 0x6a, 0x3c, 0xd8, 0x5c, 0x44, 0x1e,
        0x87, 0xfb, 0x3f, 0xab, 0xec, 0xb3, 0xb6, 0x77
    };

    h264e_rkv_nals_init(extra_info);
    h264e_rkv_stream_init(&extra_info->stream);

    extra_info->sei_buf = mpp_calloc_size(RK_U8, H264E_SEI_BUF_SIZE);
    memcpy(extra_info->sei_buf, h264e_sei_uuid, H264E_UUID_LENGTH);

    return MPP_OK;
}

static MPP_RET h264e_rkv_deinit_extra_info(void *extra_info)
{
    H264eRkvExtraInfo *info = (H264eRkvExtraInfo *)extra_info;
    h264e_rkv_stream_deinit(&info->stream);
    h264e_rkv_nals_deinit(info);

    MPP_FREE(info->sei_buf);

    return MPP_OK;
}

static MPP_RET h264e_rkv_set_extra_info(H264eHalContext *ctx)
{
    H264eRkvExtraInfo *info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &info->sps;
    H264ePps *pps = &info->pps;

    h264e_hal_enter();

    info->nal_num = 0;
    h264e_rkv_stream_reset(&info->stream);

    h264e_rkv_nal_start(info, H264E_NAL_SPS, H264E_NAL_PRIORITY_HIGHEST);
    h264e_set_sps(ctx, sps);
    h264e_rkv_sps_write(sps, &info->stream);
    h264e_rkv_nal_end(info);

    h264e_rkv_nal_start(info, H264E_NAL_PPS, H264E_NAL_PRIORITY_HIGHEST);
    h264e_set_pps(ctx, pps, sps);
    h264e_rkv_pps_write(pps, sps, &info->stream);
    h264e_rkv_nal_end(info);

    if (ctx->sei_mode == MPP_ENC_SEI_MODE_ONE_SEQ || ctx->sei_mode == MPP_ENC_SEI_MODE_ONE_FRAME) {
        info->sei_change_flg |= H264E_SEI_CHG_SPSPPS;
        h264e_rkv_nal_start(info, H264E_NAL_SEI, H264E_NAL_PRIORITY_DISPOSABLE);
        h264e_rkv_sei_encode(ctx);
        h264e_rkv_nal_end(info);
    }

    h264e_rkv_encapsulate_nals(info);

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET h264e_rkv_get_extra_info(H264eHalContext *ctx, MppPacket *pkt_out)
{
    RK_S32 k = 0;
    size_t offset = 0;
    MppPacket  pkt = ctx->packeted_param;
    H264eRkvExtraInfo *src = (H264eRkvExtraInfo *)ctx->extra_info;

    for (k = 0; k < src->nal_num; k++) {
        h264e_hal_dbg(H264E_DBG_HEADER, "get extra info nal type %d, size %d bytes", src->nal[k].i_type, src->nal[k].i_payload);
        mpp_packet_write(pkt, offset, src->nal[k].p_payload, src->nal[k].i_payload);
        offset += src->nal[k].i_payload;
    }
    mpp_packet_set_length(pkt, offset);
    *pkt_out = pkt;

    return MPP_OK;
}

static MPP_RET h264e_rkv_set_osd_plt(H264eHalContext *ctx, void *param)
{
    MppEncOSDPlt *plt = (MppEncOSDPlt *)param;
    h264e_hal_enter();
    if (plt->buf) {
        ctx->osd_plt_type = 0;
#ifdef RKPLATFORM
        if (MPP_OK != VPUClientSendReg2(ctx->vpu_fd, H264E_IOC_SET_OSD_PLT, sizeof(MppEncOSDPlt), param)) {
            h264e_hal_err("set osd plt error");
            return MPP_NOK;
        }
#endif
    } else {
        ctx->osd_plt_type = 1;
    }

    h264e_hal_leave();
    return MPP_OK;
}

static MPP_RET h264e_rkv_set_osd_data(H264eHalContext *ctx, void *param)
{
    MppEncOSDData *src = (MppEncOSDData *)param;
    MppEncOSDData *dst = &ctx->osd_data;
    RK_U32 num = src->num_region;

    h264e_hal_enter();
    if (num > 8) {
        h264e_hal_err("number of region %d exceed maxinum 8");
        return MPP_NOK;
    }

    dst->num_region = num;

    if (num) {
        if (src->buf) {
            dst->buf = src->buf;
            memcpy(dst->region, src->region, num * sizeof(MppEncOSDRegion));
        }
    }

    h264e_hal_leave();
    return MPP_OK;
}

static void h264e_rkv_reference_deinit( H264eRkvDpbCtx *dpb_ctx)
{
    H264eRkvDpbCtx *d_ctx = (H264eRkvDpbCtx *)dpb_ctx;

    h264e_hal_enter();

    MPP_FREE(d_ctx->frames.unused);

    h264e_hal_leave();
}


static void h264e_rkv_reference_init( void *dpb,  H264eHalParam *par)
{
    //RK_S32 k = 0;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)dpb;
    H264eRefParam *ref_cfg = &par->ref;

    h264e_hal_enter();
    memset(dpb_ctx, 0, sizeof(H264eRkvDpbCtx));

    dpb_ctx->frames.unused = mpp_calloc(H264eRkvFrame *, H264E_REF_MAX + 1);
    //for(k=0; k<RKV_H264E_REF_MAX+1; k++) {
    //    dpb_ctx->frames.reference[k] = &dpb_ctx->frame_buf[k];
    //    h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->frames.reference[%d]: %p", k, dpb_ctx->frames.reference[k]);
    //}
    dpb_ctx->i_long_term_reference_flag = ref_cfg->i_long_term_en;
    dpb_ctx->i_tmp_idr_pic_id = 0;

    h264e_hal_leave();
}

MPP_RET hal_h264e_rkv_init(void *hal, MppHalCfg *cfg)
{
    VPU_CLIENT_TYPE type = VPU_ENC_RKV;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvDpbCtx *dpb_ctx = NULL;
    h264e_hal_enter();


    ctx->ioctl_input    = mpp_calloc(H264eRkvIoctlInput, 1);
    ctx->ioctl_output   = mpp_calloc(H264eRkvIoctlOutput, 1);
    ctx->regs           = mpp_calloc(H264eRkvRegSet, RKV_H264E_LINKTABLE_FRAME_NUM);
    ctx->buffers        = mpp_calloc(h264e_hal_rkv_buffers, 1);
    ctx->extra_info     = mpp_calloc(H264eRkvExtraInfo, 1);
    ctx->dpb_ctx        = mpp_calloc(H264eRkvDpbCtx, 1);
    ctx->dump_files     = mpp_calloc(H264eRkvDumpFiles, 1);
    ctx->param_buf      = mpp_calloc_size(void,  H264E_EXTRA_INFO_BUF_SIZE);
    mpp_packet_init(&ctx->packeted_param, ctx->param_buf, H264E_EXTRA_INFO_BUF_SIZE);
    h264e_rkv_open_dump_files(ctx->dump_files);
    h264e_rkv_init_extra_info(ctx->extra_info);
    h264e_rkv_reference_init(ctx->dpb_ctx, &ctx->param);

    ctx->int_cb = cfg->hal_int_cb;
    ctx->frame_cnt = 0;
    ctx->frame_cnt_gen_ready = 0;
    ctx->frame_cnt_send_ready = 0;
    ctx->num_frames_to_send = 1;

    /* support multi-refs */
    dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    dpb_ctx->i_frame_cnt = 0;
    dpb_ctx->i_frame_num = 0;

    ctx->vpu_fd = -1;
    h264e_hal_dbg(H264E_DBG_DETAIL, "vpu client: %d", type);
#ifdef RKPLATFORM
    if (ctx->vpu_fd <= 0) {
        ctx->vpu_fd = VPUClientInit(type);
        if (ctx->vpu_fd <= 0) {
            h264e_hal_err("get vpu_socket(%d) <=0, failed. \n", ctx->vpu_fd);
            return MPP_ERR_UNKNOW;
        } else {
            VPUHwEncConfig_t hwCfg;
            h264e_hal_dbg(H264E_DBG_DETAIL, "get vpu_socket(%d), success. \n", ctx->vpu_fd);
            memset(&hwCfg, 0, sizeof(VPUHwEncConfig_t));
            if (VPUClientGetHwCfg(ctx->vpu_fd, (RK_U32*)&hwCfg, sizeof(hwCfg))) {
                h264e_hal_err("h264enc # Get HwCfg failed, release vpu\n");
                VPUClientRelease(ctx->vpu_fd);
                return MPP_NOK;
            }
        }
    }
#endif

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_deinit(void *hal)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->ioctl_input);
    MPP_FREE(ctx->ioctl_output);
    MPP_FREE(ctx->param_buf);

    if (ctx->buffers) {
        h264e_rkv_free_buffers(ctx);
        MPP_FREE(ctx->buffers);
    }

    if (ctx->extra_info) {
        h264e_rkv_deinit_extra_info(ctx->extra_info);
        MPP_FREE(ctx->extra_info);
    }

    if (ctx->packeted_param) {
        mpp_packet_deinit(&ctx->packeted_param);
        ctx->packeted_param = NULL;
    }

    if (ctx->dpb_ctx) {
        h264e_rkv_reference_deinit(ctx->dpb_ctx);
        MPP_FREE(ctx->dpb_ctx);
    }

    if (ctx->dump_files) {
        h264e_rkv_close_dump_files(ctx->dump_files);
        MPP_FREE(ctx->dump_files);
    }

    if (ctx->qp_p) {
        mpp_data_deinit(ctx->qp_p);
        ctx->qp_p = NULL;
    }

    if (ctx->sse_p) {
        mpp_data_deinit(ctx->sse_p);
        ctx->qp_p = NULL;
    }

    if (ctx->intra_qs) {
        mpp_linreg_deinit(ctx->intra_qs);
        ctx->intra_qs = NULL;
    }

    if (ctx->inter_qs) {
        mpp_linreg_deinit(ctx->inter_qs);
        ctx->inter_qs = NULL;
    }

#ifdef RKPLATFORM
    if (ctx->vpu_fd <= 0) {
        h264e_hal_err("invalid vpu socket: %d", ctx->vpu_fd);
        return MPP_NOK;
    }

    if (VPU_SUCCESS != VPUClientRelease(ctx->vpu_fd)) {
        h264e_hal_err("VPUClientRelease failed");
        return MPP_ERR_VPUHW;
    }
#endif


    h264e_hal_leave();
    return MPP_OK;
}

static MPP_RET
h264e_rkv_set_ioctl_extra_info(H264eRkvIoctlExtraInfo *extra_info,
                               H264eHwCfg *syn, H264eRkvRegSet *regs)
{
    H264eRkvIoctlExtraInfoElem *info = NULL;
    RK_U32 hor_stride = regs->swreg23.src_ystrid + 1;
    RK_U32 ver_stride = syn->ver_stride ? syn->ver_stride : syn->height;
    RK_U32 frame_size = hor_stride * ver_stride; // TODO: according to yuv format

    extra_info->magic = 0;
    extra_info->cnt = 2;

    /* input cb addr */
    info = &extra_info->elem[0];
    info->reg_idx = 71;
    info->offset  = frame_size;

    /* input cr addr */
    info = &extra_info->elem[1];
    info->reg_idx = 72;
    info->offset  = frame_size * 5 / 4; //TODO: relevant with YUV format
    return MPP_OK;
}

static void h264e_rkv_set_mb_rc(H264eHalContext *ctx)
{
    RK_S32 m = 0;
    RK_S32 q = 0;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncRcCfg *rc = &cfg->rc;
    H264eHwCfg *hw = &ctx->hw_cfg;
    H264eMbRcCtx *mb_rc = &ctx->mb_rc;

    if (rc->rc_mode == MPP_ENC_RC_MODE_VBR) {
        m = H264E_MB_RC_ONLY_QUALITY;
        q = rc->quality;
        if (q != MPP_ENC_RC_QUALITY_CQP) {
            /* better quality for intra frame */
            if (hw->frame_type == H264E_RKV_FRAME_I)
                q++;
            q = H264E_HAL_CLIP3(q, MPP_ENC_RC_QUALITY_WORST, MPP_ENC_RC_QUALITY_BEST);
        }
    } else { // CBR
        m = H264E_MB_RC_ONLY_BITRATE;
        if (hw->frame_type == H264E_RKV_FRAME_I)
            m--;
        m = H264E_HAL_CLIP3(m, H264E_MB_RC_ONLY_QUALITY, H264E_MB_RC_ONLY_BITRATE);
        q = -1;
    }


    mb_rc->mode = m;
    mb_rc->quality = q;
    h264e_hal_dbg(H264E_DBG_RC, "mbrc mode %d quality %d", mb_rc->mode, mb_rc->quality);
}


static MPP_RET h264e_rkv_set_rc_regs(H264eHalContext *ctx, H264eRkvRegSet *regs,
                                     H264eHwCfg *syn, RcSyntax *rc_syn)
{
    RK_U32 num_mbs_oneframe =
        (syn->width + 15) / 16 * ((syn->height + 15) / 16);
    RK_U32 mb_target_size_mul_16 = (rc_syn->bit_target << 4) / num_mbs_oneframe;
    RK_U32 mb_target_size = mb_target_size_mul_16 >> 4;
    MppEncRcCfg *rc = &ctx->cfg->rc;
    H264eMbRcCtx *mb_rc = &ctx->mb_rc;
    H264eRkvMbRcMcfg m_cfg;

    h264e_rkv_set_mb_rc(ctx);
    m_cfg = mb_rc_m_cfg[mb_rc->mode];

    /* (VBR) if focus on quality, qp range is limited more precisely */
    if (rc->rc_mode == MPP_ENC_RC_MODE_VBR) {
        if (rc->quality == MPP_ENC_RC_QUALITY_CQP) {
            syn->qp_min = syn->qp;
            syn->qp_max = syn->qp;
        } else {
            H264eRkvMbRcQcfg q_cfg = mb_rc_q_cfg[mb_rc->quality];
            syn->qp_min = q_cfg.qp_min;
            syn->qp_max = q_cfg.qp_max;
        }
    }

    regs->swreg10.pic_qp        = syn->qp;
    regs->swreg46.rc_en         = 1; //0: disable mb rc
    regs->swreg46.rc_mode       = m_cfg.aq_prop < 16; //0:frame/slice rc; 1:mbrc

    regs->swreg46.aqmode_en     = m_cfg.aq_prop && m_cfg.aq_strength;
    regs->swreg46.aq_strg       = (RK_U32)(m_cfg.aq_strength * 1.0397 * 256);
    regs->swreg54.rc_qp_mod     = 2; //sw_quality_flag;
    regs->swreg54.rc_fact0      = m_cfg.aq_prop;
    regs->swreg54.rc_fact1      = 16 - m_cfg.aq_prop;
    regs->swreg54.rc_max_qp     = syn->qp_max;
    regs->swreg54.rc_min_qp     = syn->qp_min;

    if (regs->swreg46.aqmode_en) {
        regs->swreg54.rc_max_qp = (RK_U32)MPP_MIN(syn->qp_max, (RK_S32)(syn->qp + m_cfg.qp_range));
        regs->swreg54.rc_min_qp = (RK_U32)MPP_MAX(syn->qp_min, (RK_S32)(syn->qp - m_cfg.qp_range));
        if (regs->swreg54.rc_max_qp < regs->swreg54.rc_min_qp)
            MPP_SWAP(RK_U32, regs->swreg54.rc_max_qp, regs->swreg54.rc_min_qp);
    }

    if (regs->swreg46.rc_mode) { //checkpoint rc open
        RK_U32 target = mb_target_size * m_cfg.mb_num;

        regs->swreg54.rc_qp_range    = m_cfg.qp_range;
        regs->swreg46.rc_ctu_num     = m_cfg.mb_num;
        regs->swreg55.ctu_ebits      = mb_target_size_mul_16;

        regs->swreg47.bits_error0    = (RK_S32)((pow(0.88, 4) - 1) * (double)target);
        regs->swreg47.bits_error1    = (RK_S32)((pow(0.88, 3) - 1) * (double)target);
        regs->swreg48.bits_error2    = (RK_S32)((pow(0.88, 2) - 1) * (double)target);
        regs->swreg48.bits_error3    = (RK_S32)((pow(0.88, 1) - 1) * (double)target);
        regs->swreg49.bits_error4    = (RK_S32)((pow(1.12, 1) - 1) * (double)target);
        regs->swreg49.bits_error5    = (RK_S32)((pow(1.12, 2) - 1) * (double)target);
        regs->swreg50.bits_error6    = (RK_S32)((pow(1.12, 3) - 1) * (double)target);
        regs->swreg50.bits_error7    = (RK_S32)((pow(1.12, 4) - 1) * (double)target);
        regs->swreg51.bits_error8    = (RK_S32)((pow(1.12, 5) - 1) * (double)target);

        regs->swreg52.qp_adjust0    = -4;
        regs->swreg52.qp_adjust1    = -3;
        regs->swreg52.qp_adjust2    = -2;
        regs->swreg52.qp_adjust3    = -1;
        regs->swreg52.qp_adjust4    =  0;
        regs->swreg52.qp_adjust5    =  1;
        regs->swreg53.qp_adjust6    =  2;
        regs->swreg53.qp_adjust7    =  3;
        regs->swreg53.qp_adjust8    =  4;

    }
    regs->swreg62.sli_beta_ofst     = 0;
    regs->swreg62.sli_alph_ofst     = 0;

    h264e_hal_dbg(H264E_DBG_RC, "rc_en %d rc_mode %d level %d qp %d qp_min %d qp_max %d",
                  regs->swreg46.rc_en, regs->swreg46.rc_mode, mb_rc->mode,
                  regs->swreg10.pic_qp, regs->swreg54.rc_min_qp, regs->swreg54.rc_max_qp);

    h264e_hal_dbg(H264E_DBG_RC, "aq_prop %d aq_strength %d mb_num %d qp_range %d",
                  m_cfg.aq_prop, m_cfg.aq_strength, m_cfg.mb_num, m_cfg.qp_range);
    h264e_hal_dbg(H264E_DBG_RC, "target qp %d frame_bits %d", syn->qp, rc_syn->bit_target);

    return MPP_OK;
}

static MPP_RET
h264e_rkv_set_osd_regs(H264eHalContext *ctx, H264eRkvRegSet *regs)
{
#define H264E_DEFAULT_OSD_INV_THR 15 //TODO: open interface later
    MppEncOSDData *osd_data = &ctx->osd_data;
    RK_U32 num = osd_data->num_region;

    if (num && osd_data->buf) { // enable OSD
        RK_S32 buf_fd = mpp_buffer_get_fd(osd_data->buf);

        if (buf_fd >= 0) {
            RK_U32 k = 0;
            MppEncOSDRegion *region = osd_data->region;

            regs->swreg65.osd_clk_sel    = 1;
            regs->swreg65.osd_plt_type   = ctx->osd_plt_type;

            for (k = 0; k < num; k++) {
                regs->swreg65.osd_en |= region[k].enable << k;
                regs->swreg65.osd_inv |= region[k].inverse << k;

                if (region[k].enable) {
                    regs->swreg67_osd_pos[k].lt_pos_x = region[k].start_mb_x;
                    regs->swreg67_osd_pos[k].lt_pos_y = region[k].start_mb_y;
                    regs->swreg67_osd_pos[k].rd_pos_x =
                        region[k].start_mb_x + region[k].num_mb_x - 1;
                    regs->swreg67_osd_pos[k].rd_pos_y =
                        region[k].start_mb_y + region[k].num_mb_y - 1;

                    regs->swreg68_indx_addr_i[k] =
                        buf_fd | (region[k].buf_offset << 10);
                }
            }

            if (region[0].inverse)
                regs->swreg66.osd_inv_r0 = H264E_DEFAULT_OSD_INV_THR;
            if (region[1].inverse)
                regs->swreg66.osd_inv_r1 = H264E_DEFAULT_OSD_INV_THR;
            if (region[2].inverse)
                regs->swreg66.osd_inv_r2 = H264E_DEFAULT_OSD_INV_THR;
            if (region[3].inverse)
                regs->swreg66.osd_inv_r3 = H264E_DEFAULT_OSD_INV_THR;
            if (region[4].inverse)
                regs->swreg66.osd_inv_r4 = H264E_DEFAULT_OSD_INV_THR;
            if (region[5].inverse)
                regs->swreg66.osd_inv_r5 = H264E_DEFAULT_OSD_INV_THR;
            if (region[6].inverse)
                regs->swreg66.osd_inv_r6 = H264E_DEFAULT_OSD_INV_THR;
            if (region[7].inverse)
                regs->swreg66.osd_inv_r7 = H264E_DEFAULT_OSD_INV_THR;
        }
    }

    return MPP_OK;
}

static MPP_RET h264e_rkv_set_pp_regs(H264eRkvRegSet *regs, H264eHwCfg *syn,
                                     MppEncPrepCfg *prep_cfg, MppBuffer hw_buf_w,
                                     MppBuffer hw_buf_r)
{
    RK_S32 k = 0;
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    regs->swreg14.src_cfmt        = syn->input_format;

    for (k = 0; k < 5; k++)
        regs->swreg21_scr_stbl[k] = 0;

    for (k = 0; k < 40; k++)
        regs->swreg22_h3d_tbl[k]  = h264e_h3d_tbl[k];

    if (syn->hor_stride) {
        stridey = syn->hor_stride - 1;
    } else {
        stridey = (regs->swreg19.src_rot == 1 || regs->swreg19.src_rot == 3)
                  ? (syn->height - 1) : (syn->width - 1);
        if (regs->swreg14.src_cfmt == 0 )
            stridey = (stridey + 1) * 4 - 1;
        else if (regs->swreg14.src_cfmt == 1 )
            stridey = (stridey + 1) * 3 - 1;
        else if ( regs->swreg14.src_cfmt == 2 || regs->swreg14.src_cfmt == 8
                  || regs->swreg14.src_cfmt == 9 )
            stridey = (stridey + 1) * 2 - 1;
    }
    stridec = (regs->swreg14.src_cfmt == 4 || regs->swreg14.src_cfmt == 6)
              ? stridey : ((stridey + 1) / 2 - 1);
    regs->swreg23.src_ystrid    = stridey;
    regs->swreg23.src_cstrid    = stridec;

    regs->swreg19.src_shp_y    = prep_cfg->sharpen.enable_y;
    regs->swreg19.src_shp_c    = prep_cfg->sharpen.enable_uv;
    regs->swreg19.src_shp_div  = prep_cfg->sharpen.div;
    regs->swreg19.src_shp_thld = prep_cfg->sharpen.threshold;
    regs->swreg21_scr_stbl[0]  = prep_cfg->sharpen.coef[0];
    regs->swreg21_scr_stbl[1]  = prep_cfg->sharpen.coef[1];
    regs->swreg21_scr_stbl[2]  = prep_cfg->sharpen.coef[2];
    regs->swreg21_scr_stbl[3]  = prep_cfg->sharpen.coef[3];
    regs->swreg21_scr_stbl[4]  = prep_cfg->sharpen.coef[4];

    (void)hw_buf_w;
    (void)hw_buf_r;

    return MPP_OK;
}

static RK_S32
h264e_rkv_find_best_qp(MppLinReg *ctx, MppEncH264Cfg *codec,
                       RK_S32 qp_start, RK_S32 bits)
{
    RK_S32 qp = qp_start;
    RK_S32 qp_best = qp_start;
    RK_S32 qp_min = codec->qp_min;
    RK_S32 qp_max = codec->qp_max;
    RK_S32 diff_best = INT_MAX;

    if (ctx->a == 0 && ctx->b == 0)
        return qp_best;

    h264e_hal_dbg(H264E_DBG_DETAIL, "RC: qp est target bit %d\n", bits);
    if (bits <= 0) {
        qp_best = mpp_clip(qp_best + codec->qp_max_step, qp_min, qp_max);
    } else {
        do {
            RK_S32 est_bits = mpp_quadreg_calc(ctx, QP2Qstep(qp));
            RK_S32 diff = est_bits - bits;
            h264e_hal_dbg(H264E_DBG_DETAIL,
                          "RC: qp est qp %d qstep %f bit %d diff %d best %d\n",
                          qp, QP2Qstep(qp), bits, diff, diff_best);
            if (MPP_ABS(diff) < MPP_ABS(diff_best)) {
                diff_best = MPP_ABS(diff);
                qp_best = qp;
                if (diff > 0)
                    qp++;
                else
                    qp--;
            } else
                break;
        } while (qp <= qp_max && qp >= qp_min);
        qp_best = mpp_clip(qp_best, qp_min, qp_max);
    }

    return qp_best;
}

static MPP_RET
h264e_rkv_update_hw_cfg(H264eHalContext *ctx, HalEncTask *task,
                        H264eHwCfg *hw_cfg)
{
    RK_S32 i;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    RcSyntax *rc_syn = (RcSyntax *)task->syntax.data;

    /* preprocess setup */
    if (prep->change) {
        RK_U32 change = prep->change;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            hw_cfg->width   = prep->width;
            hw_cfg->height  = prep->height;
            hw_cfg->input_format = prep->format;
            hw_cfg->hor_stride = prep->hor_stride;
            hw_cfg->ver_stride = prep->ver_stride;

            h264e_rkv_set_format(hw_cfg, prep);
        }

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
            switch (prep->color) {
            case MPP_FRAME_SPC_RGB : {
                /* BT.601 */
                /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
                 * Cb = 0.5647 (B - Y) + 128
                 * Cr = 0.7132 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            case MPP_FRAME_SPC_BT709 : {
                /* BT.709 */
                /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
                 * Cb = 0.5389 (B - Y) + 128
                 * Cr = 0.6350 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 13933;
                hw_cfg->color_conversion_coeff_b = 46871;
                hw_cfg->color_conversion_coeff_c = 4732;
                hw_cfg->color_conversion_coeff_e = 35317;
                hw_cfg->color_conversion_coeff_f = 41615;
            } break;
            default : {
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            }
        }

        prep->change = 0;
    }

    if (codec->change) {
        // TODO: setup sps / pps here
        hw_cfg->idr_pic_id = !ctx->idr_pic_id;
        hw_cfg->filter_disable = codec->deblock_disable;
        hw_cfg->slice_alpha_offset = codec->deblock_offset_alpha;
        hw_cfg->slice_beta_offset = codec->deblock_offset_beta;
        hw_cfg->inter4x4_disabled = (codec->profile >= 31) ? (1) : (0);
        hw_cfg->cabac_init_idc = codec->cabac_init_idc;
        hw_cfg->qp = codec->qp_init;

        hw_cfg->qp_prev = hw_cfg->qp;

        codec->change = 0;
    }

    /* init qp calculate, if outside doesn't set init qp.
     * mpp will use bpp to estimates one.
     */
    if (hw_cfg->qp <= 0) {
        RK_S32 qp_tbl[2][13] = {
            {
                26, 36, 48, 63, 85, 110, 152, 208, 313, 427, 936,
                1472, 0x7fffffff
            },
            {42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12, 9, 6}
        };
        RK_S32 pels = ctx->cfg->prep.width * ctx->cfg->prep.height;
        RK_S32 bits_per_pic = axb_div_c(rc->bps_target,
                                        rc->fps_out_denorm,
                                        rc->fps_out_num);

        if (pels) {
            RK_S32 upscale = 8000;
            if (bits_per_pic > 1000000)
                hw_cfg->qp = codec->qp_min;
            else {
                RK_S32 j = -1;

                pels >>= 8;
                bits_per_pic >>= 5;

                bits_per_pic *= pels + 250;
                bits_per_pic /= 350 + (3 * pels) / 4;
                bits_per_pic = axb_div_c(bits_per_pic, upscale, pels << 6);

                while (qp_tbl[0][++j] < bits_per_pic);

                hw_cfg->qp = qp_tbl[1][j];
                hw_cfg->qp_prev = hw_cfg->qp;
            }
        }
    }

    if (NULL == ctx->intra_qs)
        mpp_linreg_init(&ctx->intra_qs, MPP_MIN(rc->gop, 15), 4);
    if (NULL == ctx->inter_qs)
        mpp_linreg_init(&ctx->inter_qs, MPP_MIN(rc->gop, 15), 4);
    if (NULL == ctx->qp_p)
        mpp_data_init(&ctx->qp_p, MPP_MIN(rc->gop, 10));
    if (NULL == ctx->sse_p)
        mpp_data_init(&ctx->sse_p, MPP_MIN(rc->gop, 10));
    mpp_assert(ctx->intra_qs);
    mpp_assert(ctx->inter_qs);
    mpp_assert(ctx->qp_p);
    mpp_assert(ctx->sse_p);

    /* frame type and rate control setup */
    h264e_hal_dbg(H264E_DBG_DETAIL,
                  "RC: qp calc ctx %p qp [%d %d] prev %d target bit %d\n",
                  ctx->inter_qs, codec->qp_min, codec->qp_max, hw_cfg->qp_prev,
                  rc_syn->bit_target);

    {
        RK_S32 prev_frame_type = hw_cfg->frame_type;
        RK_S32 is_cqp = rc->rc_mode ==
                        MPP_ENC_RC_MODE_VBR && rc->quality == MPP_ENC_RC_QUALITY_CQP;

        if (rc_syn->type == INTRA_FRAME) {
            hw_cfg->frame_type = H264E_RKV_FRAME_I;
            hw_cfg->coding_type = RKVENC_CODING_TYPE_IDR;
            hw_cfg->frame_num = 0;

            if (!is_cqp) {
                if (ctx->frame_cnt > 0)
                    hw_cfg->qp = mpp_data_avg(ctx->qp_p, -1, 1, 1) - 3;

                /*
                 * Previous frame is inter then intra frame can not
                 * have a big qp step between these two frames
                 */
                if (prev_frame_type == H264E_RKV_FRAME_P)
                    hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 2,
                                          hw_cfg->qp_prev + 2);
            }
        } else {
            hw_cfg->frame_type = H264E_RKV_FRAME_P;
            hw_cfg->coding_type = RKVENC_CODING_TYPE_P;

            if (!is_cqp) {
                RK_S32 sse = mpp_data_avg(ctx->sse_p, 1, 1, 1) + 1;
                hw_cfg->qp = h264e_rkv_find_best_qp(ctx->inter_qs, codec,
                                                    hw_cfg->qp_prev,
                                                    rc_syn->bit_target * 1024 / sse);

                /*
                 * Previous frame is intra then inter frame can not
                 * have a big qp step between these two frames
                 */
                if (prev_frame_type == H264E_RKV_FRAME_I)
                    hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 4,
                                          hw_cfg->qp_prev + 4);
            }
        }
    }

    h264e_hal_dbg(H264E_DBG_DETAIL, "RC: qp calc ctx %p qp get %d\n",
                  ctx->inter_qs, hw_cfg->qp);

    hw_cfg->qp = mpp_clip(hw_cfg->qp,
                          hw_cfg->qp_prev - codec->qp_max_step,
                          hw_cfg->qp_prev + codec->qp_max_step);

    h264e_hal_dbg(H264E_DBG_DETAIL,
                  "RC: qp calc ctx %p qp clip %d prev %d step %d\n",
                  ctx->inter_qs, hw_cfg->qp,
                  hw_cfg->qp_prev, codec->qp_max_step);

    hw_cfg->qp_prev = hw_cfg->qp;

    hw_cfg->mad_qp_delta = 0;
    hw_cfg->mad_threshold = 6;
    hw_cfg->keyframe_max_interval = rc->gop ? rc->gop : 1;
    hw_cfg->qp_min = codec->qp_min;
    hw_cfg->qp_max = codec->qp_max;

    /* disable mb rate control first */
    hw_cfg->cp_distance_mbs = 0;
    for (i = 0; i < 10; i++)
        hw_cfg->cp_target[i] = 0;

    for (i = 0; i < 9; i++)
        hw_cfg->target_error[i] = 0;

    for (i = 0; i < 9; i++)
        hw_cfg->delta_qp[i] = 0;

    /* slice mode setup */
    hw_cfg->slice_size_mb_rows = (prep->width + 15) >> 4;

    /* input and preprocess config */
    hw_cfg->input_luma_addr = mpp_buffer_get_fd(task->input);
    hw_cfg->input_cb_addr = hw_cfg->input_luma_addr;
    hw_cfg->input_cr_addr = hw_cfg->input_cb_addr;
    hw_cfg->output_strm_limit_size = mpp_buffer_get_size(task->output);
    hw_cfg->output_strm_addr = mpp_buffer_get_fd(task->output);

    return MPP_OK;
}


MPP_RET hal_h264e_rkv_gen_regs(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvRegSet *regs = NULL;
    H264eRkvIoctlRegInfo *ioctl_reg_info = NULL;
    H264eHwCfg *syn = &ctx->hw_cfg;
    H264eRkvIoctlInput *ioctl_info = (H264eRkvIoctlInput *)ctx->ioctl_input;
    H264eRkvRegSet *reg_list = (H264eRkvRegSet *)ctx->regs;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264ePps *pps = &extra_info->pps;
    HalEncTask *enc_task = &task->enc;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;

    RK_S32 pic_width_align16 = 0;
    RK_S32 pic_height_align16 = 0;
    RK_S32 pic_width_in_blk64 = 0;
    h264e_hal_rkv_buffers *bufs = (h264e_hal_rkv_buffers *)ctx->buffers;
    RK_U32 buf2_idx = ctx->frame_cnt % 2;

    MppBuffer mv_info_buf = task->enc.mv_info;

    ctx->enc_mode = RKV_H264E_ENC_MODE;

    h264e_hal_enter();

    enc_task->flags.err = 0;

    h264e_rkv_update_hw_cfg(ctx, &task->enc, syn);
    h264e_rkv_dump_mpp_syntax_in(syn, ctx);

    pic_width_align16 = (syn->width + 15) & (~15);
    pic_height_align16 = (syn->height + 15) & (~15);
    pic_width_in_blk64 = (syn->width + 63) / 64;

    h264e_rkv_adjust_param(ctx); //TODO: future expansion

    h264e_hal_dbg(H264E_DBG_SIMPLE,
                  "frame %d | type %d | start gen regs",
                  ctx->frame_cnt, syn->frame_type);

    if (ctx->sei_mode == MPP_ENC_SEI_MODE_ONE_FRAME
        && extra_info->sei_change_flg) {
        extra_info->nal_num = 0;
        h264e_rkv_stream_reset(&extra_info->stream);
        h264e_rkv_nal_start(extra_info, H264E_NAL_SEI,
                            H264E_NAL_PRIORITY_DISPOSABLE);
        h264e_rkv_sei_encode(ctx);
        h264e_rkv_nal_end(extra_info);
    }

    if (ctx->frame_cnt == 0) {
        if (MPP_OK != h264e_rkv_allocate_buffers(ctx, syn)) {
            h264e_hal_err("h264e_rkv_allocate_buffers failed, free buffers and return");
            enc_task->flags.err |= HAL_ENC_TASK_ERR_ALLOC;
            h264e_rkv_free_buffers(ctx);
            return MPP_ERR_MALLOC;
        }
    }

    if (ctx->enc_mode == 2 || ctx->enc_mode == 3) { //link table mode
        RK_U32 idx = ctx->frame_cnt_gen_ready;
        ctx->num_frames_to_send = RKV_H264E_LINKTABLE_EACH_NUM;
        if (idx == 0) {
            ioctl_info->enc_mode = ctx->enc_mode;
            ioctl_info->frame_num = ctx->num_frames_to_send;
        }
        regs = &reg_list[idx];
        ioctl_info->reg_info[idx].reg_num = sizeof(H264eRkvRegSet) / 4;
        ioctl_reg_info = &ioctl_info->reg_info[idx];
    } else {
        ctx->num_frames_to_send = 1;
        ioctl_info->frame_num = ctx->num_frames_to_send;
        ioctl_info->enc_mode = ctx->enc_mode;
        regs = &reg_list[0];
        ioctl_info->reg_info[0].reg_num = sizeof(H264eRkvRegSet) / 4;
        ioctl_reg_info = &ioctl_info->reg_info[0];
    }

    if (MPP_OK != h264e_rkv_reference_frame_set(ctx, syn)) {
        h264e_hal_err("h264e_rkv_reference_frame_set failed, multi-ref error");
    }

    h264e_hal_dbg(H264E_DBG_DETAIL,
                  "generate regs when frame_cnt_gen_ready/(num_frames_to_send-1): %d/%d",
                  ctx->frame_cnt_gen_ready, ctx->num_frames_to_send - 1);

    memset(regs, 0, sizeof(H264eRkvRegSet));

    regs->swreg01.rkvenc_ver      = 0x1;

    regs->swreg02.lkt_num = 0;
    regs->swreg02.rkvenc_cmd = ctx->enc_mode;
    //regs->swreg02.rkvenc_cmd = syn->link_table_en ? 0x2 : 0x1;
    regs->swreg02.enc_cke = 0;

    regs->swreg03.safe_clr = 0x0;

    regs->swreg04.lkt_addr = 0x10000000;

    regs->swreg05.ofe_fnsh    = 1;
    regs->swreg05.lkt_fnsh    = 1;
    regs->swreg05.clr_fnsh    = 1;
    regs->swreg05.ose_fnsh    = 1;
    regs->swreg05.bs_ovflr    = 1;
    regs->swreg05.brsp_ful    = 1;
    regs->swreg05.brsp_err    = 1;
    regs->swreg05.rrsp_err    = 1;
    regs->swreg05.tmt_err     = 1;

    regs->swreg09.pic_wd8_m1  = pic_width_align16 / 8 - 1;
    regs->swreg09.pic_wfill   = (syn->width & 0xf)
                                ? (16 - (syn->width & 0xf)) : 0;
    regs->swreg09.pic_hd8_m1  = pic_height_align16 / 8 - 1;
    regs->swreg09.pic_hfill   = (syn->height & 0xf)
                                ? (16 - (syn->height & 0xf)) : 0;

    regs->swreg10.enc_stnd       = 0; //H264
    regs->swreg10.cur_frm_ref    = 1; //current frame will be refered
    regs->swreg10.bs_scp         = 1;
    regs->swreg10.slice_int      = 0;
    regs->swreg10.node_int       = 0;

    regs->swreg11.ppln_enc_lmt     = 0xf;
    regs->swreg11.rfp_load_thrd    = 0;

    regs->swreg12.src_bus_ordr     = 0x0;
    regs->swreg12.cmvw_bus_ordr    = 0x0;
    regs->swreg12.dspw_bus_ordr    = 0x0;
    regs->swreg12.rfpw_bus_ordr    = 0x0;
    regs->swreg12.src_bus_edin     = 0x0;
    regs->swreg12.meiw_bus_edin    = 0x0;
    regs->swreg12.bsw_bus_edin     = 0x7;
    regs->swreg12.lktr_bus_edin    = 0x0;
    regs->swreg12.ctur_bus_edin    = 0x0;
    regs->swreg12.lktw_bus_edin    = 0x0;
    regs->swreg12.rfp_map_dcol     = 0x0;
    regs->swreg12.rfp_map_sctr     = 0x0;

    regs->swreg13.axi_brsp_cke      = 0x7f;
    regs->swreg13.cime_dspw_orsd    = 0x0;

    h264e_rkv_set_pp_regs(regs, syn, &ctx->set->prep,
                          bufs->hw_pp_buf[buf2_idx], bufs->hw_pp_buf[1 - buf2_idx]);
    h264e_rkv_set_ioctl_extra_info(&ioctl_reg_info->extra_info, syn, regs);

    regs->swreg24_adr_srcy     = syn->input_luma_addr;
    regs->swreg25_adr_srcu     = syn->input_cb_addr;
    regs->swreg26_adr_srcv     = syn->input_cr_addr;

    regs->swreg30_rfpw_addr    = mpp_buffer_get_fd(dpb_ctx->fdec->hw_buf);
    if (dpb_ctx->fref[0][0])
        regs->swreg31_rfpr_addr = mpp_buffer_get_fd(dpb_ctx->fref[0][0]->hw_buf);

    if (bufs->hw_dsp_buf[buf2_idx])
        regs->swreg34_dspw_addr = mpp_buffer_get_fd(bufs->hw_dsp_buf[buf2_idx]);
    if (bufs->hw_dsp_buf[1 - buf2_idx])
        regs->swreg35_dspr_addr =
            mpp_buffer_get_fd(bufs->hw_dsp_buf[1 - buf2_idx]);

    if (mv_info_buf) {
        regs->swreg10.mei_stor    = 1;
        regs->swreg36_meiw_addr   = mpp_buffer_get_fd(mv_info_buf);
    } else {
        regs->swreg10.mei_stor    = 0;
        regs->swreg36_meiw_addr   = 0;
    }

    regs->swreg38_bsbb_addr    = syn->output_strm_addr;
    if (VPUClientGetIOMMUStatus() > 0)
        /* TODO: stream size relative with syntax */
        regs->swreg37_bsbt_addr = regs->swreg38_bsbb_addr
                                  | (syn->output_strm_limit_size << 10);
    else
        regs->swreg37_bsbt_addr = regs->swreg38_bsbb_addr
                                  + (syn->output_strm_limit_size);
    regs->swreg39_bsbr_addr    = regs->swreg38_bsbb_addr;
    regs->swreg40_bsbw_addr    = regs->swreg38_bsbb_addr;

    h264e_hal_dbg(H264E_DBG_DPB, "regs->swreg37_bsbt_addr: %08x",
                  regs->swreg37_bsbt_addr);
    h264e_hal_dbg(H264E_DBG_DPB, "regs->swreg38_bsbb_addr: %08x",
                  regs->swreg38_bsbb_addr);
    h264e_hal_dbg(H264E_DBG_DPB, "regs->swreg39_bsbr_addr: %08x",
                  regs->swreg39_bsbr_addr);
    h264e_hal_dbg(H264E_DBG_DPB, "regs->swreg40_bsbw_addr: %08x",
                  regs->swreg40_bsbw_addr);

    regs->swreg41.sli_cut         = 0;
    regs->swreg41.sli_cut_mode    = 0;
    regs->swreg41.sli_cut_bmod    = 1;
    regs->swreg41.sli_max_num     = 0;
    regs->swreg41.sli_out_mode    = 0;
    regs->swreg41.sli_cut_cnum    = 0;

    regs->swreg42.sli_cut_byte    = 0;

    {
        RK_U32 cime_wid_4p = 0, cime_hei_4p = 0;
        if (sps->i_level_idc == 10 || sps->i_level_idc == 9) { //9 is level 1b;
            cime_wid_4p = 44;
            cime_hei_4p = 12;
        } else if ( sps->i_level_idc == 11 || sps->i_level_idc == 12
                    || sps->i_level_idc == 13 || sps->i_level_idc == 20 ) {
            cime_wid_4p = 44;
            cime_hei_4p = 28;
        } else {
            cime_wid_4p = 44;
            cime_hei_4p = 28;
        }

        if (176 < cime_wid_4p * 4)
            cime_wid_4p = 176 / 4;

        if (112 < cime_hei_4p * 4)
            cime_hei_4p = 112 / 4;

        if (cime_hei_4p / 4 * 2 > (RK_U32)(regs->swreg09.pic_hd8_m1 + 2) / 2)
            cime_hei_4p = (regs->swreg09.pic_hd8_m1 + 2) / 2 / 2 * 4;

        if (cime_wid_4p / 4 > (RK_U32)(((regs->swreg09.pic_wd8_m1 + 1)
                                        * 8 + 63) / 64 * 64 / 128 * 4))
            cime_wid_4p = ((regs->swreg09.pic_wd8_m1 + 1)
                           * 8 + 63) / 64 * 64 / 128 * 4 * 4;

        regs->swreg43.cime_srch_h    = cime_wid_4p / 4;
        regs->swreg43.cime_srch_v    = cime_hei_4p / 4;
    }

    regs->swreg43.rime_srch_h    = 7;
    regs->swreg43.rime_srch_v    = 5;
    regs->swreg43.dlt_frm_num    = 0x0;

    regs->swreg44.pmv_mdst_h    = 5;
    regs->swreg44.pmv_mdst_v    = 5;
    regs->swreg44.mv_limit      = (sps->i_level_idc > 20)
                                  ? 2 : ((sps->i_level_idc >= 11) ? 1 : 0);
    regs->swreg44.mv_num        = 3;

    if (pic_width_align16 > 3584)
        regs->swreg45.cime_rama_h = 8;
    else if (pic_width_align16 > 3136)
        regs->swreg45.cime_rama_h = 9;
    else if (pic_width_align16 > 2816)
        regs->swreg45.cime_rama_h = 10;
    else if (pic_width_align16 > 2560)
        regs->swreg45.cime_rama_h = 11;
    else if (pic_width_align16 > 2368)
        regs->swreg45.cime_rama_h = 12;
    else if (pic_width_align16 > 2176)
        regs->swreg45.cime_rama_h = 13;
    else if (pic_width_align16 > 2048)
        regs->swreg45.cime_rama_h = 14;
    else if (pic_width_align16 > 1856)
        regs->swreg45.cime_rama_h = 15;
    else if (pic_width_align16 > 1792)
        regs->swreg45.cime_rama_h = 16;
    else
        regs->swreg45.cime_rama_h = 17;

    {
        RK_U32 i_swin_all_4_h = (2 * regs->swreg43.cime_srch_v + 1);
        RK_U32 i_swin_all_16_w = ((regs->swreg43.cime_srch_h * 4 + 15)
                                  / 16 * 2 + 1);
        if (i_swin_all_4_h < regs->swreg45.cime_rama_h)
            regs->swreg45.cime_rama_max = (i_swin_all_4_h - 1)
                                          * pic_width_in_blk64 + i_swin_all_16_w;
        else
            regs->swreg45.cime_rama_max = (regs->swreg45.cime_rama_h - 1)
                                          * pic_width_in_blk64 + i_swin_all_16_w;
    }

    regs->swreg45.cach_l1_dtmr     = 0x3;
    regs->swreg45.cach_l2_tag      = 0x0;
    if (pic_width_align16 <= 512)
        regs->swreg45.cach_l2_tag  = 0x0;
    else if (pic_width_align16 <= 1024)
        regs->swreg45.cach_l2_tag  = 0x1;
    else if (pic_width_align16 <= 2048)
        regs->swreg45.cach_l2_tag  = 0x2;
    else if (pic_width_align16 <= 4096)
        regs->swreg45.cach_l2_tag  = 0x3;

    h264e_rkv_set_rc_regs(ctx, regs, syn, (RcSyntax *)enc_task->syntax.data);

    regs->swreg56.rect_size = (sps->i_profile_idc == H264_PROFILE_BASELINE
                               && sps->i_level_idc <= 30);
    regs->swreg56.inter_4x4 = 1;
    regs->swreg56.arb_sel   = 0;
    regs->swreg56.vlc_lmt   = (sps->i_profile_idc < H264_PROFILE_HIGH
                               && !pps->b_cabac);
    regs->swreg56.rdo_mark  = 0;

    {
        RK_U32 i_nal_type = 0, i_nal_ref_idc = 0;

        /* TODO: extend syn->frame_coding_type definition */
        if (syn->coding_type == RKVENC_CODING_TYPE_IDR ) {
            /* reset ref pictures */
            i_nal_type    = H264E_NAL_SLICE_IDR;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGHEST;
        } else if (syn->coding_type == RKVENC_CODING_TYPE_I ) {
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        } else if (syn->coding_type == RKVENC_CODING_TYPE_P ) {
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        } else if (syn->coding_type == RKVENC_CODING_TYPE_BREF ) {
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH;
        } else { /* B frame */
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_DISPOSABLE;
        }
        if (sps->keyframe_max_interval == 1)
            i_nal_ref_idc = H264E_NAL_PRIORITY_LOW;

        regs->swreg57.nal_ref_idc      = i_nal_ref_idc;
        regs->swreg57.nal_unit_type    = i_nal_type;
    }

    regs->swreg58.max_fnum    = sps->i_log2_max_frame_num - 4;
    regs->swreg58.drct_8x8    = 1;
    regs->swreg58.mpoc_lm4    = sps->i_log2_max_poc_lsb - 4;

    regs->swreg59.etpy_mode       = pps->b_cabac;
    regs->swreg59.trns_8x8        = pps->b_transform_8x8_mode;
    regs->swreg59.csip_flg        = pps->b_constrained_intra_pred;
    regs->swreg59.num_ref0_idx    = pps->i_num_ref_idx_l0_default_active - 1;
    regs->swreg59.num_ref1_idx    = pps->i_num_ref_idx_l1_default_active - 1;
    regs->swreg59.pic_init_qp     = pps->i_pic_init_qp - H264_QP_BD_OFFSET;
    regs->swreg59.cb_ofst         = pps->i_chroma_qp_index_offset;
    regs->swreg59.cr_ofst         = pps->i_second_chroma_qp_index_offset;
    regs->swreg59.wght_pred       = 0x0;
    regs->swreg59.dbf_cp_flg      = 1;

    regs->swreg60.sli_type        = syn->frame_type;
    regs->swreg60.pps_id          = pps->i_id;
    regs->swreg60.drct_smvp       = 0x0;
    regs->swreg60.num_ref_ovrd    = 0;
    regs->swreg60.cbc_init_idc    = syn->cabac_init_idc;

    regs->swreg60.frm_num         = dpb_ctx->i_frame_num;

    regs->swreg61.idr_pid    = dpb_ctx->i_idr_pic_id;
    regs->swreg61.poc_lsb    = dpb_ctx->fdec->i_poc
                               & ((1 << sps->i_log2_max_poc_lsb) - 1);

    regs->swreg62.rodr_pic_idx      = dpb_ctx->ref_pic_list_order[0][0].idc;
    regs->swreg62.ref_list0_rodr    = dpb_ctx->b_ref_pic_list_reordering[0];
    regs->swreg62.dis_dblk_idc      = 0;
    regs->swreg62.rodr_pic_num      = dpb_ctx->ref_pic_list_order[0][0].arg;

    {
        RK_S32 mmco4_pre = dpb_ctx->i_mmco_command_count > 0
                           && (dpb_ctx->mmco[0].memory_management_control_operation == 4);
        regs->swreg63.nopp_flg     = 0x0;
        regs->swreg63.ltrf_flg     = dpb_ctx->i_long_term_reference_flag;
        regs->swreg63.arpm_flg     = dpb_ctx->i_mmco_command_count;
        regs->swreg63.mmco4_pre    = mmco4_pre;
        regs->swreg63.mmco_0       = dpb_ctx->i_mmco_command_count > mmco4_pre
                                     ? dpb_ctx->mmco[mmco4_pre].memory_management_control_operation : 0;
        regs->swreg63.dopn_m1_0    = dpb_ctx->i_mmco_command_count > mmco4_pre
                                     ? dpb_ctx->mmco[mmco4_pre].i_difference_of_pic_nums - 1 : 0;

        regs->swreg64.mmco_1       = dpb_ctx->i_mmco_command_count
                                     > (mmco4_pre + 1)
                                     ? dpb_ctx->mmco[(mmco4_pre + 1)].memory_management_control_operation : 0;
        regs->swreg64.dopn_m1_1    = dpb_ctx->i_mmco_command_count
                                     > (mmco4_pre + 1)
                                     ? dpb_ctx->mmco[(mmco4_pre + 1)].i_difference_of_pic_nums - 1 : 0;
    }

    h264e_rkv_set_osd_regs(ctx, regs);

    regs->swreg69.bs_lgth    = 0x0;

    regs->swreg70.sse_l32    = 0x0;

    regs->swreg71.qp_sum     = 0x0;
    regs->swreg71.sse_h8     = 0x0;

    regs->swreg72.slice_scnum    = 0x0;
    regs->swreg72.slice_slnum    = 0x0;

    regs->swreg73.st_enc      = 0x0;
    regs->swreg73.axiw_cln    = 0x0;
    regs->swreg73.axir_cln    = 0x0;

    regs->swreg74.fnum_enc    = 0x0;
    regs->swreg74.fnum_cfg    = 0x0;
    regs->swreg74.fnum_int    = 0x0;

    regs->swreg75.node_addr    = 0x0;

    regs->swreg76.bsbw_addr    = 0x0;
    regs->swreg76.Bsbw_ovfl    = 0x0;

    h264e_rkv_dump_mpp_reg_in(ctx);

    h264e_rkv_reference_frame_update(ctx);
    dpb_ctx->i_frame_cnt++;
    if (dpb_ctx->i_nal_ref_idc != H264E_NAL_PRIORITY_DISPOSABLE)
        dpb_ctx->i_frame_num ++;

    ctx->frame_cnt_gen_ready++;
    ctx->frame_cnt++;
    extra_info->sei.frame_cnt++;
    hw_cfg->frame_num++;

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvRegSet *reg_list = (H264eRkvRegSet *)ctx->regs;
    RK_U32 length = 0, k = 0;
    H264eRkvIoctlInput *ioctl_info = (H264eRkvIoctlInput *)ctx->ioctl_input;
    HalEncTask *enc_task = &task->enc;

    h264e_hal_enter();
    if (enc_task->flags.err) {
        h264e_hal_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        h264e_hal_dbg(H264E_DBG_DETAIL,
                      "frame_cnt_gen_ready(%d) != num_frames_to_send(%d), start hardware later",
                      ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    }

    h264e_hal_dbg(H264E_DBG_DETAIL,
                  "memcpy %d frames' regs from reg list to reg info",
                  ioctl_info->frame_num);
    for (k = 0; k < ioctl_info->frame_num; k++)
        memcpy(&ioctl_info->reg_info[k].regs, &reg_list[k],
               sizeof(H264eRkvRegSet));

    length = (sizeof(ioctl_info->enc_mode) + sizeof(ioctl_info->frame_num) +
              sizeof(ioctl_info->reg_info[0]) * ioctl_info->frame_num) >> 2;

    ctx->frame_cnt_send_ready ++;

    (void)task;

#ifdef RKPLATFORM
    if (ctx->vpu_fd <= 0) {
        h264e_hal_err("invalid vpu socket: %d", ctx->vpu_fd);
        return MPP_NOK;
    }

    h264e_hal_dbg(H264E_DBG_DETAIL, "vpu client is sending %d regs", length);
    if (MPP_OK != VPUClientSendReg(ctx->vpu_fd, (RK_U32 *)ioctl_info, length)) {
        h264e_hal_err("VPUClientSendReg Failed!!!");
        return  MPP_ERR_VPUHW;
    } else {
        h264e_hal_dbg(H264E_DBG_DETAIL, "VPUClientSendReg successfully!");
    }
#else
    (void)length;
#endif

    h264e_hal_leave();

    return ret;
}

static MPP_RET h264e_rkv_set_feedback(h264e_feedback *fb, H264eRkvIoctlOutput *out)
{
    RK_U32 k = 0;
    H264eRkvIoctlOutputElem *elem = NULL;

    h264e_hal_enter();
    for (k = 0; k < out->frame_num; k++) {
        elem = &out->elem[k];
        fb->qp_sum = elem->swreg71.qp_sum;
        fb->out_strm_size = elem->swreg69.bs_lgth;
        fb->sse_sum = elem->swreg70.sse_l32 +
                      ((RK_S64)(elem->swreg71.sse_h8 & 0xff) << 32);

        fb->hw_status = 0;
        h264e_hal_dbg(H264E_DBG_DETAIL, "hw_status: 0x%08x", elem->hw_status);
        if (elem->hw_status & RKV_H264E_INT_LINKTABLE_FINISH) {
            h264e_hal_err("RKV_H264E_INT_LINKTABLE_FINISH");
        }
        if (elem->hw_status & RKV_H264E_INT_ONE_FRAME_FINISH) {
            h264e_hal_dbg(H264E_DBG_DETAIL, "RKV_H264E_INT_ONE_FRAME_FINISH");
        }
        if (elem->hw_status & RKV_H264E_INT_ONE_SLICE_FINISH) {
            h264e_hal_err("RKV_H264E_INT_ONE_SLICE_FINISH");
        }

        if (elem->hw_status & RKV_H264E_INT_SAFE_CLEAR_FINISH) {
            h264e_hal_err("RKV_H264E_INT_SAFE_CLEAR_FINISH");
        }

        if (elem->hw_status & RKV_H264E_INT_BIT_STREAM_OVERFLOW) {
            h264e_hal_err("RKV_H264E_INT_BIT_STREAM_OVERFLOW");
            fb->hw_status = 1;
        }
        if (elem->hw_status & RKV_H264E_INT_BUS_WRITE_FULL) {
            h264e_hal_err("RKV_H264E_INT_BUS_WRITE_FULL");
            fb->hw_status = 1;
        }
        if (elem->hw_status & RKV_H264E_INT_BUS_WRITE_ERROR) {
            h264e_hal_err("RKV_H264E_INT_BUS_WRITE_ERROR");
            fb->hw_status = 1;
        }
        if (elem->hw_status & RKV_H264E_INT_BUS_READ_ERROR) {
            h264e_hal_err("RKV_H264E_INT_BUS_READ_ERROR");
            fb->hw_status = 1;
        }
        if (elem->hw_status & RKV_H264E_INT_TIMEOUT_ERROR) {
            h264e_hal_err("RKV_H264E_INT_TIMEOUT_ERROR");
            fb->hw_status = 1;
        }

        fb->hw_status = elem->hw_status;
    }

    h264e_hal_leave();

    return MPP_OK;
}

/* this function must be called after first VPUClientWaitResult,
 * since it depend on h264e_feedback structure, and this structure will
 * be filled after VPUClientWaitResult called
 */
static MPP_RET h264e_rkv_resend(H264eHalContext *ctx, RK_S32 mb_rc)
{
    unsigned int k = 0;
    H264eRkvIoctlInput *ioctl_info = (H264eRkvIoctlInput *)ctx->ioctl_input;
    H264eRkvRegSet *reg_list = (H264eRkvRegSet *)ctx->regs;
    RK_S32 length;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    RK_S32 num_mb = MPP_ALIGN(prep->width, 16)
                    * MPP_ALIGN(prep->height, 16) / 16 / 16;
    VPU_CMD_TYPE cmd = 0;
    H264eRkvIoctlOutput *reg_out = (H264eRkvIoctlOutput *)ctx->ioctl_output;
    h264e_feedback *fb = &ctx->feedback;
    RK_S32 hw_ret = 0;

    reg_list->swreg10.pic_qp        = fb->qp_sum / num_mb;
    reg_list->swreg46.rc_en         = mb_rc; //0: disable mb rc
    reg_list->swreg46.rc_mode       = mb_rc ? 1 : 0; //0:frame/slice rc; 1:mbrc
    reg_list->swreg46.aqmode_en     = mb_rc;

    for (k = 0; k < ioctl_info->frame_num; k++)
        memcpy(&ioctl_info->reg_info[k].regs,
               &reg_list[k], sizeof(H264eRkvRegSet));

    length = (sizeof(ioctl_info->enc_mode) +
              sizeof(ioctl_info->frame_num) +
              sizeof(ioctl_info->reg_info[0]) *
              ioctl_info->frame_num) >> 2;

#ifdef RKPLATFORM
    if (MPP_OK != VPUClientSendReg(ctx->vpu_fd, (RK_U32 *)ioctl_info, length)) {
        h264e_hal_err("VPUClientSendReg Failed!!!");
        return MPP_ERR_VPUHW;
    } else {
        h264e_hal_dbg(H264E_DBG_DETAIL, "VPUClientSendReg successfully!");
    }

    hw_ret = VPUClientWaitResult(ctx->vpu_fd,
                                 (RK_U32 *)reg_out,
                                 length, &cmd, NULL);
#endif
    if ((VPU_SUCCESS != hw_ret) || (cmd != VPU_SEND_CONFIG_ACK_OK))
        h264e_hal_err("hardware wait error");

    if (hw_ret != MPP_OK) {
        h264e_hal_err("hardware returns error:%d", hw_ret);
        return MPP_ERR_VPUHW;
    }

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_wait(void *hal, HalTaskInfo *task)
{
    VPU_CMD_TYPE cmd = 0;
    RK_S32 hw_ret = 0;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvIoctlOutput *reg_out = (H264eRkvIoctlOutput *)ctx->ioctl_output;
    RK_S32 length = (sizeof(reg_out->frame_num)
                     + sizeof(reg_out->elem[0])
                     * ctx->num_frames_to_send) >> 2;
    IOInterruptCB int_cb = ctx->int_cb;
    h264e_feedback *fb = &ctx->feedback;
    HalEncTask *enc_task = &task->enc;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RK_S32 num_mb = MPP_ALIGN(prep->width, 16)
                    * MPP_ALIGN(prep->height, 16) / 16 / 16;
    /* for dumping ratecontrol message */
    RcSyntax *rc_syn = (RcSyntax *)task->enc.syntax.data;

    h264e_hal_enter();

    if (enc_task->flags.err) {
        h264e_hal_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        h264e_hal_dbg(H264E_DBG_DETAIL,
                      "frame_cnt_gen_ready(%d) != num_frames_to_send(%d), wait hardware later",
                      ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    } else {
        ctx->frame_cnt_gen_ready = 0;
        if (ctx->enc_mode == 3) {
            /* TODO: remove later */
            h264e_hal_dbg(H264E_DBG_DETAIL, "only for test enc_mode 3 ...");
            if (ctx->frame_cnt_send_ready != RKV_H264E_LINKTABLE_FRAME_NUM) {
                h264e_hal_dbg(H264E_DBG_DETAIL,
                              "frame_cnt_send_ready(%d) != RKV_H264E_LINKTABLE_FRAME_NUM(%d), wait hardware later",
                              ctx->frame_cnt_send_ready, RKV_H264E_LINKTABLE_FRAME_NUM);
                return MPP_OK;
            } else {
                ctx->frame_cnt_send_ready = 0;
            }
        }
    }

#ifdef RKPLATFORM
    if (ctx->vpu_fd <= 0) {
        h264e_hal_err("invalid vpu socket: %d", ctx->vpu_fd);
        return MPP_NOK;
    }

    h264e_hal_dbg(H264E_DBG_DETAIL, "VPUClientWaitResult expect length %d\n",
                  length);

    hw_ret = VPUClientWaitResult(ctx->vpu_fd, (RK_U32 *)reg_out,
                                 length, &cmd, NULL);

    h264e_hal_dbg(H264E_DBG_DETAIL,
                  "VPUClientWaitResult: ret %d, cmd %d, len %d\n",
                  hw_ret, cmd, length);


    if ((VPU_SUCCESS != hw_ret) || (cmd != VPU_SEND_CONFIG_ACK_OK))
        h264e_hal_err("hardware wait error");

    if (hw_ret != MPP_OK) {
        h264e_hal_err("hardware returns error:%d", hw_ret);
        return MPP_ERR_VPUHW;
    }
#else
    (void)hw_ret;
    (void)length;
    (void)cmd;
#endif

    h264e_rkv_set_feedback(fb, reg_out);

    /* we need re-encode */
    if (ctx->frame_cnt == 1) {
        h264e_rkv_resend(ctx, 0);

        h264e_rkv_set_feedback(fb, reg_out);
    }

    task->enc.length = fb->out_strm_size;
    h264e_hal_dbg(H264E_DBG_DETAIL, "output stream size %d\n",
                  fb->out_strm_size);
    if (int_cb.callBack) {
        RcSyntax *syn = (RcSyntax *)task->enc.syntax.data;
        RcHalResult result;
        double avg_qp = 0.0;
        RK_S32 avg_sse = 1;
        RK_S32 wlen = 15;
        RK_S32 prev_sse = 0;

        avg_qp = fb->qp_sum * 1.0 / num_mb;

        if (syn->type == INTER_P_FRAME) {
            avg_sse = (RK_S32)sqrt((double)(fb->sse_sum));
            prev_sse = mpp_data_avg(ctx->sse_p, 1, 1, 1);
            if (avg_sse > prev_sse)
                wlen = wlen * prev_sse / avg_sse;
            else
                wlen = wlen * avg_sse / prev_sse;
        }

        mpp_assert(avg_qp >= 0);
        mpp_assert(avg_qp <= 51);

        result.bits = fb->out_strm_size * 8;
        result.type = syn->type;
        fb->result = &result;

        /* dump rc message */
        h264e_hal_dbg(H264E_DBG_RC, "target bits %d real bits %d "
                      "target qp %d real qp %0.2f\n",
                      rc_syn->bit_target, result.bits,
                      hw_cfg->qp, avg_qp);
        if (h264e_hal_log_mode & H264E_DBG_DUMP_RC) {
            FILE *fp = fopen("/tmp/rc_log.txt", "ab+");
            if (fp) {
                fprintf(fp, "bits: %d %d qp %d %f\n",
                        rc_syn->bit_target, result.bits,
                        hw_cfg->qp, avg_qp);
                fclose(fp);
            } else
                mpp_err("can not open rc log file");
        }

        if (syn->type == INTER_P_FRAME) {
            mpp_save_regdata(ctx->inter_qs, QP2Qstep(avg_qp),
                             result.bits * 1024 / avg_sse);
            mpp_quadreg_update(ctx->inter_qs, wlen);
        }

        hw_cfg->qp_prev = avg_qp;

        if (syn->type == INTER_P_FRAME) {
            mpp_data_update(ctx->qp_p, avg_qp);
            mpp_data_update(ctx->sse_p, avg_sse);
        }

        int_cb.callBack(int_cb.opaque, fb);
        h264e_hal_dbg(H264E_DBG_RC, "real qp %0.2f frame_bits %d",
                      avg_qp, result.bits);
    }

    h264e_rkv_dump_mpp_reg_out(ctx);
    h264e_rkv_dump_mpp_feedback(ctx);
    h264e_rkv_dump_mpp_strm_out(ctx, enc_task->output);
    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_reset(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_flush(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_control(void *hal, RK_S32 cmd_type, void *param)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    h264e_hal_dbg(H264E_DBG_DETAIL, "h264e_rkv_control cmd 0x%x, info %p",
                  cmd_type, param);
    switch (cmd_type) {
    case MPP_ENC_SET_EXTRA_INFO: {
        break;
    }
    case MPP_ENC_GET_EXTRA_INFO: {
        MppPacket *pkt_out = (MppPacket *)param;
        h264e_rkv_set_extra_info(ctx);
        h264e_rkv_get_extra_info(ctx, pkt_out);
        h264e_rkv_dump_mpp_strm_out_header(ctx, *pkt_out);
        break;
    }
    case MPP_ENC_SET_OSD_PLT_CFG: {
        h264e_rkv_set_osd_plt(ctx, param);
        break;
    }
    case MPP_ENC_SET_OSD_DATA_CFG: {
        h264e_rkv_set_osd_data(ctx, param);
        break;
    }
    case MPP_ENC_SET_SEI_CFG: {
        ctx->sei_mode = *((MppEncSeiMode *)param);
        break;
    }
    case MPP_ENC_GET_SEI_DATA: {
        h264e_rkv_get_extra_info(ctx, (MppPacket *)param);
        break;
    }
    case MPP_ENC_SET_PREP_CFG: {
        //LKSTODO: check cfg
        break;
    }
    case MPP_ENC_SET_RC_CFG : {
        // TODO: do rate control check here
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        MppEncH264Cfg *src = &ctx->set->codec.h264;
        MppEncH264Cfg *dst = &ctx->cfg->codec.h264;
        RK_U32 change = src->change;

        // TODO: do codec check first

        if (change & MPP_ENC_H264_CFG_STREAM_TYPE)
            dst->stream_type = src->stream_type;
        if (change & MPP_ENC_H264_CFG_CHANGE_PROFILE) {
            dst->profile = src->profile;
            dst->level = src->level;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_ENTROPY) {
            dst->entropy_coding_mode = src->entropy_coding_mode;
            dst->cabac_init_idc = src->cabac_init_idc;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_TRANS_8x8)
            dst->transform8x8_mode = src->transform8x8_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CONST_INTRA)
            dst->constrained_intra_pred_mode = src->constrained_intra_pred_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CHROMA_QP) {
            dst->chroma_cb_qp_offset = src->chroma_cb_qp_offset;
            dst->chroma_cr_qp_offset = src->chroma_cr_qp_offset;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_DEBLOCKING) {
            dst->deblock_disable = src->deblock_disable;
            dst->deblock_offset_alpha = src->deblock_offset_alpha;
            dst->deblock_offset_beta = src->deblock_offset_beta;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_LONG_TERM)
            dst->use_longterm = src->use_longterm;
        if (change & MPP_ENC_H264_CFG_CHANGE_QP_LIMIT) {
            dst->qp_init = src->qp_init;
            dst->qp_max = src->qp_max;
            dst->qp_min = src->qp_min;
            dst->qp_max_step = src->qp_max_step;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH) {
            dst->intra_refresh_mode = src->intra_refresh_mode;
            dst->intra_refresh_arg = src->intra_refresh_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SLICE_MODE) {
            dst->slice_mode = src->slice_mode;
            dst->slice_arg = src->slice_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_VUI) {
            dst->vui = src->vui;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SEI) {
            dst->sei = src->sei;
        }

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    } break;
    default : {
        h264e_hal_err("unrecognizable cmd type %x", cmd_type);
    } break;
    }

    h264e_hal_leave();
    return MPP_OK;
}

