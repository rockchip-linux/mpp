/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "vepu5xx_common"

#include <string.h>

#include "mpp_debug.h"
#include "mpp_common.h"
#include "vepu5xx_common.h"

const VepuRgb2YuvCfg vepu_rgb2limit_yuv_cfg_set[] = {
    /* MPP_BT601_FULL_RGB_TO_LIMIT_YUV */
    {
        .color = MPP_FRAME_SPC_RGB, .dst_range = MPP_FRAME_RANGE_UNSPECIFIED,
        ._2y = {.r_coeff = 66, .g_coeff = 129, .b_coeff = 25, .offset = 16},
        ._2u = {.r_coeff = -38, .g_coeff = -74, .b_coeff = 112, .offset = 128},
        ._2v = {.r_coeff = 112, .g_coeff = -94, .b_coeff = -18, .offset = 128},
    },
    /* MPP_BT709_FULL_RGB_TO_LIMIT_YUV */
    {
        .color = MPP_FRAME_SPC_BT709, .dst_range = MPP_FRAME_RANGE_UNSPECIFIED,
        ._2y = {.r_coeff = 47, .g_coeff = 157, .b_coeff = 16, .offset = 16},
        ._2u = {.r_coeff = -26, .g_coeff = -87, .b_coeff = 112, .offset = 128},
        ._2v = {.r_coeff = 112, .g_coeff = -102, .b_coeff = -10, .offset = 128},
    },
};

const VepuRgb2YuvCfg vepu_rgb2full_yuv_cfg_set[] = {
    /* MPP_BT601_FULL_RGB_TO_FULL_YUV */
    {
        .color = MPP_FRAME_SPC_RGB, .dst_range = MPP_FRAME_RANGE_JPEG,
        ._2y = {.r_coeff = 77, .g_coeff = 150, .b_coeff = 29, .offset = 0},
        ._2u = {.r_coeff = -43, .g_coeff = -85, .b_coeff = 128, .offset = 128},
        ._2v = {.r_coeff = 128, .g_coeff = -107, .b_coeff = -21, .offset = 128},
    },
    /* MPP_BT709_FULL_RGB_TO_FULL_YUV */
    {
        .color = MPP_FRAME_SPC_BT709, .dst_range = MPP_FRAME_RANGE_JPEG,
        ._2y = {.r_coeff = 54, .g_coeff = 183, .b_coeff = 18, .offset = 0},
        ._2u = {.r_coeff = -29, .g_coeff = -99, .b_coeff = 128, .offset = 128},
        ._2v = {.r_coeff = 128, .g_coeff = -116, .b_coeff = -12, .offset = 128},
    },
};

const VepuRgb2YuvCfg *get_rgb2yuv_cfg(MppFrameColorRange range, MppFrameColorSpace color)
{
    const VepuRgb2YuvCfg *cfg;
    RK_U32 size;
    RK_U32 i;

    /* only jpeg full range, others limit range */
    if (range == MPP_FRAME_RANGE_JPEG) {
        /* set default cfg BT.601 */
        cfg = &vepu_rgb2full_yuv_cfg_set[0];
        size = MPP_ARRAY_ELEMS(vepu_rgb2full_yuv_cfg_set);
    } else {
        /* set default cfg BT.601 */
        cfg = &vepu_rgb2limit_yuv_cfg_set[0];
        size = MPP_ARRAY_ELEMS(vepu_rgb2limit_yuv_cfg_set);
    }

    for (i = 0; i < size; i++)
        if (cfg[i].color == color)
            return &cfg[i];

    return cfg;
}

const RK_U32 vepu580_540_h264_flat_scl_tab[576] = {
    /* 0x2200 */
    0x2fbe3333, 0x2fbe4189, 0x2fbe3333, 0x2fbe4189, 0x2ca42fbe, 0x2ca43c79, 0x2ca42fbe, 0x2ca43c79,
    0x3c794189, 0x3c7951ec, 0x3c794189, 0x3c7951ec, 0x2ca42fbe, 0x2ca43c79, 0x2ca42fbe, 0x2ca43c79,
    0x2fbe3333, 0x2fbe4189, 0x2fbe3333, 0x2fbe4189, 0x2ca42fbe, 0x2ca43c79, 0x2ca42fbe, 0x2ca43c79,
    0x3c794189, 0x3c7951ec, 0x3c794189, 0x3c7951ec, 0x2ca42fbe, 0x2ca43c79, 0x2ca42fbe, 0x2ca43c79,
    0x2b322e8c, 0x2b323a84, 0x2b322e8c, 0x2b323a84, 0x2a4a2b32, 0x2a4a37d2, 0x2a4a2b32, 0x2a4a37d2,
    0x37d23a84, 0x37d24ae6, 0x37d23a84, 0x37d24ae6, 0x2a4a2b32, 0x2a4a37d2, 0x2a4a2b32, 0x2a4a37d2,
    0x2b322e8c, 0x2b323a84, 0x2b322e8c, 0x2b323a84, 0x2a4a2b32, 0x2a4a37d2, 0x2a4a2b32, 0x2a4a37d2,
    0x37d23a84, 0x37d24ae6, 0x37d23a84, 0x37d24ae6, 0x2a4a2b32, 0x2a4a37d2, 0x2a4a2b32, 0x2a4a37d2,
    0x25cb2762, 0x25cb31a6, 0x25cb2762, 0x25cb31a6, 0x22ef25cb, 0x22ef2ed1, 0x22ef25cb, 0x22ef2ed1,
    0x2ed131a6, 0x2ed13e6a, 0x2ed131a6, 0x2ed13e6a, 0x22ef25cb, 0x22ef2ed1, 0x22ef25cb, 0x22ef2ed1,
    0x25cb2762, 0x25cb31a6, 0x25cb2762, 0x25cb31a6, 0x22ef25cb, 0x22ef2ed1, 0x22ef25cb, 0x22ef2ed1,
    0x2ed131a6, 0x2ed13e6a, 0x2ed131a6, 0x2ed13e6a, 0x22ef25cb, 0x22ef2ed1, 0x22ef25cb, 0x22ef2ed1,
    0x22e32492, 0x22e32ed0, 0x22e32492, 0x22e32ed0, 0x202422e3, 0x20242bfb, 0x202422e3, 0x20242bfb,
    0x2bfb2ed0, 0x2bfb3a41, 0x2bfb2ed0, 0x2bfb3a41, 0x202422e3, 0x20242bfb, 0x202422e3, 0x20242bfb,
    0x22e32492, 0x22e32ed0, 0x22e32492, 0x22e32ed0, 0x202422e3, 0x20242bfb, 0x202422e3, 0x20242bfb,
    0x2bfb2ed0, 0x2bfb3a41, 0x2bfb2ed0, 0x2bfb3a41, 0x202422e3, 0x20242bfb, 0x202422e3, 0x20242bfb,
    0x1e3c2000, 0x1e3c28f6, 0x1e3c2000, 0x1e3c28f6, 0x1cb21e3c, 0x1cb22631, 0x1cb21e3c, 0x1cb22631,
    0x263128f6, 0x26313367, 0x263128f6, 0x26313367, 0x1cb21e3c, 0x1cb22631, 0x1cb21e3c, 0x1cb22631,
    0x1e3c2000, 0x1e3c28f6, 0x1e3c2000, 0x1e3c28f6, 0x1cb21e3c, 0x1cb22631, 0x1cb21e3c, 0x1cb22631,
    0x263128f6, 0x26313367, 0x263128f6, 0x26313367, 0x1cb21e3c, 0x1cb22631, 0x1cb21e3c, 0x1cb22631,
    0x1aae1c72, 0x1aae239e, 0x1aae1c72, 0x1aae239e, 0x191c1aae, 0x191c21c0, 0x191c1aae, 0x191c21c0,
    0x21c0239e, 0x21c02d32, 0x21c0239e, 0x21c02d32, 0x191c1aae, 0x191c21c0, 0x191c1aae, 0x191c21c0,
    0x1aae1c72, 0x1aae239e, 0x1aae1c72, 0x1aae239e, 0x191c1aae, 0x191c21c0, 0x191c1aae, 0x191c21c0,
    0x21c0239e, 0x21c02d32, 0x21c0239e, 0x21c02d32, 0x191c1aae, 0x191c21c0, 0x191c1aae, 0x191c21c0,
    0x00be0033, 0x00be0089, 0x00be0033, 0x00be0089, 0x00a400be, 0x00a40079, 0x00a400be, 0x00a40079,
    0x00790089, 0x007900ec, 0x00790089, 0x007900ec, 0x00a400be, 0x00a40079, 0x00a400be, 0x00a40079,
    0x00be0033, 0x00be0089, 0x00be0033, 0x00be0089, 0x00a400be, 0x00a40079, 0x00a400be, 0x00a40079,
    0x00790089, 0x007900ec, 0x00790089, 0x007900ec, 0x00a400be, 0x00a40079, 0x00a400be, 0x00a40079,
    0x0032008c, 0x00320084, 0x0032008c, 0x00320084, 0x004a0032, 0x004a00d2, 0x004a0032, 0x004a00d2,
    0x00d20084, 0x00d200e6, 0x00d20084, 0x00d200e6, 0x004a0032, 0x004a00d2, 0x004a0032, 0x004a00d2,
    0x0032008c, 0x00320084, 0x0032008c, 0x00320084, 0x004a0032, 0x004a00d2, 0x004a0032, 0x004a00d2,
    0x00d20084, 0x00d200e6, 0x00d20084, 0x00d200e6, 0x004a0032, 0x004a00d2, 0x004a0032, 0x004a00d2,
    0x00cb0062, 0x00cb00a6, 0x00cb0062, 0x00cb00a6, 0x00ef00cb, 0x00ef00d1, 0x00ef00cb, 0x00ef00d1,
    0x00d100a6, 0x00d1006a, 0x00d100a6, 0x00d1006a, 0x00ef00cb, 0x00ef00d1, 0x00ef00cb, 0x00ef00d1,
    0x00cb0062, 0x00cb00a6, 0x00cb0062, 0x00cb00a6, 0x00ef00cb, 0x00ef00d1, 0x00ef00cb, 0x00ef00d1,
    0x00d100a6, 0x00d1006a, 0x00d100a6, 0x00d1006a, 0x00ef00cb, 0x00ef00d1, 0x00ef00cb, 0x00ef00d1,
    0x00e30092, 0x00e300d0, 0x00e30092, 0x00e300d0, 0x002400e3, 0x002400fb, 0x002400e3, 0x002400fb,
    0x00fb00d0, 0x00fb0041, 0x00fb00d0, 0x00fb0041, 0x002400e3, 0x002400fb, 0x002400e3, 0x002400fb,
    0x00e30092, 0x00e300d0, 0x00e30092, 0x00e300d0, 0x002400e3, 0x002400fb, 0x002400e3, 0x002400fb,
    0x00fb00d0, 0x00fb0041, 0x00fb00d0, 0x00fb0041, 0x002400e3, 0x002400fb, 0x002400e3, 0x002400fb,
    0x003c0000, 0x003c00f6, 0x003c0000, 0x003c00f6, 0x00b2003c, 0x00b20031, 0x00b2003c, 0x00b20031,
    0x003100f6, 0x00310067, 0x003100f6, 0x00310067, 0x00b2003c, 0x00b20031, 0x00b2003c, 0x00b20031,
    0x003c0000, 0x003c00f6, 0x003c0000, 0x003c00f6, 0x00b2003c, 0x00b20031, 0x00b2003c, 0x00b20031,
    0x003100f6, 0x00310067, 0x003100f6, 0x00310067, 0x00b2003c, 0x00b20031, 0x00b2003c, 0x00b20031,
    0x00ae0072, 0x00ae009e, 0x00ae0072, 0x00ae009e, 0x001c00ae, 0x001c00c0, 0x001c00ae, 0x001c00c0,
    0x00c0009e, 0x00c00032, 0x00c0009e, 0x00c00032, 0x001c00ae, 0x001c00c0, 0x001c00ae, 0x001c00c0,
    0x00ae0072, 0x00ae009e, 0x00ae0072, 0x00ae009e, 0x001c00ae, 0x001c00c0, 0x001c00ae, 0x001c00c0,
    0x00c0009e, 0x00c00032, 0x00c0009e, 0x00c00032, 0x001c00ae, 0x001c00c0, 0x001c00ae, 0x001c00c0,
    0x002f0033, 0x002f0041, 0x002f0033, 0x002f0041, 0x002c002f, 0x002c003c, 0x002c002f, 0x002c003c,
    0x003c0041, 0x003c0051, 0x003c0041, 0x003c0051, 0x002c002f, 0x002c003c, 0x002c002f, 0x002c003c,
    0x002f0033, 0x002f0041, 0x002f0033, 0x002f0041, 0x002c002f, 0x002c003c, 0x002c002f, 0x002c003c,
    0x003c0041, 0x003c0051, 0x003c0041, 0x003c0051, 0x002c002f, 0x002c003c, 0x002c002f, 0x002c003c,
    0x002b002e, 0x002b003a, 0x002b002e, 0x002b003a, 0x002a002b, 0x002a0037, 0x002a002b, 0x002a0037,
    0x0037003a, 0x0037004a, 0x0037003a, 0x0037004a, 0x002a002b, 0x002a0037, 0x002a002b, 0x002a0037,
    0x002b002e, 0x002b003a, 0x002b002e, 0x002b003a, 0x002a002b, 0x002a0037, 0x002a002b, 0x002a0037,
    0x0037003a, 0x0037004a, 0x0037003a, 0x0037004a, 0x002a002b, 0x002a0037, 0x002a002b, 0x002a0037,
    0x01250127, 0x01250131, 0x01250127, 0x01250131, 0x01220125, 0x0122012e, 0x01220125, 0x0122012e,
    0x012e0131, 0x012e013e, 0x012e0131, 0x012e013e, 0x01220125, 0x0122012e, 0x01220125, 0x0122012e,
    0x01250127, 0x01250131, 0x01250127, 0x01250131, 0x01220125, 0x0122012e, 0x01220125, 0x0122012e,
    0x012e0131, 0x012e013e, 0x012e0131, 0x012e013e, 0x01220125, 0x0122012e, 0x01220125, 0x0122012e,
    0x01220124, 0x0122012e, 0x01220124, 0x0122012e, 0x01200122, 0x0120012b, 0x01200122, 0x0120012b,
    0x012b012e, 0x012b013a, 0x012b012e, 0x012b013a, 0x01200122, 0x0120012b, 0x01200122, 0x0120012b,
    0x01220124, 0x0122012e, 0x01220124, 0x0122012e, 0x01200122, 0x0120012b, 0x01200122, 0x0120012b,
    0x012b012e, 0x012b013a, 0x012b012e, 0x012b013a, 0x01200122, 0x0120012b, 0x01200122, 0x0120012b,
    0x001e0020, 0x001e0028, 0x001e0020, 0x001e0028, 0x001c001e, 0x001c0026, 0x001c001e, 0x001c0026,
    0x00260028, 0x00260033, 0x00260028, 0x00260033, 0x001c001e, 0x001c0026, 0x001c001e, 0x001c0026,
    0x001e0020, 0x001e0028, 0x001e0020, 0x001e0028, 0x001c001e, 0x001c0026, 0x001c001e, 0x001c0026,
    0x00260028, 0x00260033, 0x00260028, 0x00260033, 0x001c001e, 0x001c0026, 0x001c001e, 0x001c0026,
    0x001a001c, 0x001a0023, 0x001a001c, 0x001a0023, 0x0019001a, 0x00190021, 0x0019001a, 0x00190021,
    0x00210023, 0x0021002d, 0x00210023, 0x0021002d, 0x0019001a, 0x00190021, 0x0019001a, 0x00190021,
    0x001a001c, 0x001a0023, 0x001a001c, 0x001a0023, 0x0019001a, 0x00190021, 0x0019001a, 0x00190021,
    0x00210023, 0x0021002d, 0x00210023, 0x0021002d, 0x0019001a, 0x00190021, 0x0019001a, 0x00190021,
};

const RK_U32 klut_weight[24] = {
    0x50800080, 0x00330000, 0xA1000100, 0x00660000, 0x42000200, 0x00CC0001,
    0x84000400, 0x01980002, 0x08000800, 0x03300005, 0x10001000, 0x0660000A,
    0x20002000, 0x0CC00014, 0x40004000, 0x19800028, 0x80008000, 0x33000050,
    0x00010000, 0x660000A1, 0x00020000, 0xCC000142, 0xFF83FFFF, 0x000001FF
};

const RK_U32 lamd_satd_qp[52] = {
    0x00000183, 0x000001b2, 0x000001e7, 0x00000223, 0x00000266, 0x000002b1, 0x00000305, 0x00000364,
    0x000003ce, 0x00000445, 0x000004cb, 0x00000562, 0x0000060a, 0x000006c8, 0x0000079c, 0x0000088b,
    0x00000996, 0x00000ac3, 0x00000c14, 0x00000d8f, 0x00000f38, 0x00001115, 0x0000132d, 0x00001586,
    0x00001829, 0x00001b1e, 0x00001e70, 0x0000222b, 0x0000265a, 0x00002b0c, 0x00003052, 0x0000363c,
    0x00003ce1, 0x00004455, 0x00004cb4, 0x00005618, 0x000060a3, 0x00006c79, 0x000079c2, 0x000088ab,
    0x00009967, 0x0000ac30, 0x0000c147, 0x0000d8f2, 0x0000f383, 0x00011155, 0x000132ce, 0x00015861,
    0x0001828d, 0x0001b1e4, 0x0001e706, 0x000222ab
};

const RK_U32 lamd_moda_qp[52] = {
    0x00000049, 0x0000005c, 0x00000074, 0x00000092, 0x000000b8, 0x000000e8, 0x00000124, 0x00000170,
    0x000001cf, 0x00000248, 0x000002df, 0x0000039f, 0x0000048f, 0x000005bf, 0x0000073d, 0x0000091f,
    0x00000b7e, 0x00000e7a, 0x0000123d, 0x000016fb, 0x00001cf4, 0x0000247b, 0x00002df6, 0x000039e9,
    0x000048f6, 0x00005bed, 0x000073d1, 0x000091ec, 0x0000b7d9, 0x0000e7a2, 0x000123d7, 0x00016fb2,
    0x0001cf44, 0x000247ae, 0x0002df64, 0x00039e89, 0x00048f5c, 0x0005bec8, 0x00073d12, 0x00091eb8,
    0x000b7d90, 0x000e7a23, 0x00123d71, 0x0016fb20, 0x001cf446, 0x00247ae1, 0x002df640, 0x0039e88c,
    0x0048f5c3, 0x005bec81, 0x0073d119, 0x0091eb85
};

const RK_U32 lamd_modb_qp[52] = {
    0x00000070, 0x00000089, 0x000000b0, 0x000000e0, 0x00000112, 0x00000160, 0x000001c0, 0x00000224,
    0x000002c0, 0x00000380, 0x00000448, 0x00000580, 0x00000700, 0x00000890, 0x00000b00, 0x00000e00,
    0x00001120, 0x00001600, 0x00001c00, 0x00002240, 0x00002c00, 0x00003800, 0x00004480, 0x00005800,
    0x00007000, 0x00008900, 0x0000b000, 0x0000e000, 0x00011200, 0x00016000, 0x0001c000, 0x00022400,
    0x0002c000, 0x00038000, 0x00044800, 0x00058000, 0x00070000, 0x00089000, 0x000b0000, 0x000e0000,
    0x00112000, 0x00160000, 0x001c0000, 0x00224000, 0x002c0000, 0x00380000, 0x00448000, 0x00580000,
    0x00700000, 0x00890000, 0x00b00000, 0x00e00000
};

const RK_U32 lamd_satd_qp_510[52] = {
    0x00000243, 0x00000289, 0x000002DA, 0x00000333, 0x00000397, 0x00000408, 0x00000487, 0x00000514,
    0x000005B5, 0x00000667, 0x000007C9, 0x000008BD, 0x00000B52, 0x00000CB5, 0x00000E44, 0x00001002,
    0x000011F9, 0x0000142D, 0x00001523, 0x000017BA, 0x00001AA2, 0x0000199F, 0x00001CC2, 0x00002049,
    0x0000243C, 0x000032D8, 0x00002DA8, 0x0000333F, 0x00003985, 0x00004092, 0x00004879, 0x000065B0,
    0x000062EC, 0x000088AA, 0x00007CA2, 0x0000AC30, 0x00009D08, 0x0000B043, 0x0000C5D9, 0x0000EF29,
    0x00010C74, 0x00012D54, 0x000121E9, 0x00014569, 0x00018BB4, 0x0001BC2A, 0x0001F28E, 0x00020490,
    0x000243D3, 0x00028AD4, 0x0002DA89, 0x000333FF,
};

static const RK_S32 zeros[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static VepuFmtCfg vepu5xx_yuv_cfg[MPP_FMT_YUV_BUTT] = {
    { /* MPP_FMT_YUV420SP */
        .format = VEPU5xx_FMT_YUV420SP,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV420SP_10BIT */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422SP */
        .format = VEPU5xx_FMT_YUV422SP,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422SP_10BIT */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV420P */
        .format = VEPU5xx_FMT_YUV420P,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV420SP_VU   */
        .format = VEPU5xx_FMT_YUV420SP,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422P */
        .format = VEPU5xx_FMT_YUV422P,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422SP_VU */
        .format = VEPU5xx_FMT_YUV422SP,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422_YUYV */
        .format = VEPU5xx_FMT_YUYV422,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422_YVYU */
        .format = VEPU5xx_FMT_YUYV422,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422_UYVY */
        .format = VEPU5xx_FMT_UYVY422,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV422_VYUY */
        .format = VEPU5xx_FMT_UYVY422,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV400 */
        .format = VEPU5xx_FMT_YUV400,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV440SP */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV411SP */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_YUV444SP */
        .format = VEPU5xx_FMT_YUV444SP,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },

    { /* MPP_FMT_YUV444P */
        .format = VEPU5xx_FMT_YUV444P,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },

    { /* MPP_FMT_YUV444SP_10BIT */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },

    { /* MPP_FMT_AYUV2BPP */
        .format = VEPU5xx_FMT_AYUV2BPP,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },

    { /* MPP_FMT_AYUV1BPP */
        .format = VEPU5xx_FMT_AYUV1BPP,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
};

static VepuFmtCfg vepu5xx_rgb_cfg[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB] = {
    { /* MPP_FMT_RGB565 */
        .format = VEPU5xx_FMT_BGR565,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_range = 0,
        .src_endian = 1,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_BGR565 */
        .format = VEPU5xx_FMT_BGR565,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 1,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_RGB555 */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_BGR555 */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_RGB444 */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_BGR444 */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_RGB888 */
        .format = VEPU5xx_FMT_BGR888,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_BGR888 */
        .format = VEPU5xx_FMT_BGR888,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_RGB101010 */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_BGR101010 */
        .format = VEPU5xx_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_ARGB8888 */
        .format = VEPU5xx_FMT_BGRA8888,
        .alpha_swap = 1,
        .rbuv_swap = 1,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_ABGR8888 */
        .format = VEPU5xx_FMT_BGRA8888,
        .alpha_swap = 1,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_BGRA8888 */
        .format = VEPU5xx_FMT_BGRA8888,
        .alpha_swap = 0,
        .rbuv_swap = 0,
        .src_range = 0,
        .src_endian = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_RGBA8888 */
        .format = VEPU5xx_FMT_BGRA8888,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_endian = 0,
        .src_range = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_ARGB4444 */
        .format = VEPU5xx_FMT_ARGB4444,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_endian = 0,
        .src_range = 0,
        .weight = zeros,
        .offset = zeros,
    },
    { /* MPP_FMT_ARGB1555 */
        .format = VEPU5xx_FMT_ARGB1555,
        .alpha_swap = 0,
        .rbuv_swap = 1,
        .src_endian = 0,
        .src_range = 0,
        .weight = zeros,
        .offset = zeros,
    },
};

MPP_RET copy2osd2(MppEncOSDData2* dst, MppEncOSDData *src1, MppEncOSDData2 *src2)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;

    if (src1) {
        dst->num_region = src1->num_region;
        for (i = 0; i < src1->num_region; i++) {
            dst->region[i].enable       = src1->region[i].enable;
            dst->region[i].inverse      = src1->region[i].inverse;
            dst->region[i].start_mb_x   = src1->region[i].start_mb_x;
            dst->region[i].start_mb_y   = src1->region[i].start_mb_y;
            dst->region[i].num_mb_x     = src1->region[i].num_mb_x;
            dst->region[i].num_mb_y     = src1->region[i].num_mb_y;
            dst->region[i].buf_offset   = src1->region[i].buf_offset;
            dst->region[i].buf          = src1->buf;
        }
        ret = MPP_OK;
    } else if (src2) {
        memcpy(dst, src2, sizeof(MppEncOSDData2));
        ret = MPP_OK;
    } else {
        ret = MPP_NOK;
    }
    return ret;
}

MPP_RET vepu5xx_set_fmt(VepuFmtCfg * cfg, MppFrameFormat format)
{
    VepuFmtCfg *fmt = NULL;
    MPP_RET ret = MPP_OK;

    format &= MPP_FRAME_FMT_MASK;

    if (MPP_FRAME_FMT_IS_YUV(format))
        fmt = &vepu5xx_yuv_cfg[format - MPP_FRAME_FMT_YUV];
    else if (MPP_FRAME_FMT_IS_RGB(format))
        fmt = &vepu5xx_rgb_cfg[format - MPP_FRAME_FMT_RGB];
    else {
        memset(cfg, 0, sizeof(*cfg));
        cfg->format = VEPU5xx_FMT_BUTT;
    }

    if (fmt && fmt->format != VEPU5xx_FMT_BUTT)
        memcpy(cfg, fmt, sizeof(*cfg));
    else {
        mpp_err_f("unsupport frame format %x\n", format);
        cfg->format = VEPU5xx_FMT_BUTT;
        ret = MPP_NOK;
    }

    return ret;
}