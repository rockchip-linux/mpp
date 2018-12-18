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

#ifndef __MPP_RGA_H__
#define __MPP_RGA_H__

/* NOTE: RGA support sync mode and async mode. We use sync mode only.  */
#define RGA_BLIT_SYNC       0x5017
#define RGA_BLIT_ASYNC      0x5018
#define RGA_FLUSH           0x5019
#define RGA_GET_RESULT      0x501a

typedef enum RgaFormat_e {
    RGA_FMT_RGBA_8888       = 0x0,
    RGA_FMT_RGBX_8888       = 0x1,
    RGA_FMT_RGB_888         = 0x2,
    RGA_FMT_BGRA_8888       = 0x3,
    RGA_FMT_RGB_565         = 0x4,
    RGA_FMT_RGBA_5551       = 0x5,
    RGA_FMT_RGBA_4444       = 0x6,
    RGA_FMT_BGR_888         = 0x7,

    RGA_FMT_YCbCr_422_SP    = 0x8,
    RGA_FMT_YCbCr_422_P     = 0x9,
    RGA_FMT_YCbCr_420_SP    = 0xa,
    RGA_FMT_YCbCr_420_P     = 0xb,

    RGA_FMT_YCrCb_422_SP    = 0xc,
    RGA_FMT_YCrCb_422_P     = 0xd,
    RGA_FMT_YCrCb_420_SP    = 0xe,
    RGA_FMT_YCrCb_420_P     = 0xf,

    RGA_FMT_BPP1            = 0x10,
    RGA_FMT_BPP2            = 0x11,
    RGA_FMT_BPP4            = 0x12,
    RGA_FMT_BPP8            = 0x13,
    RGA_FMT_BUTT,
} RgaFormat;

typedef struct RgaImg_t {
    RK_ULONG    yrgb_addr;       /* yrgb    addr         */
    RK_ULONG    uv_addr;         /* cb/cr   addr         */
    RK_ULONG    v_addr;          /* cr      addr         */
    RK_U32      format;          // definition by RgaFormat

    RK_U16      act_w;           // width
    RK_U16      act_h;           // height
    RK_U16      x_offset;        // offset from left
    RK_U16      y_offset;        // offset from top

    RK_U16      vir_w;           // horizontal stride
    RK_U16      vir_h;           // vertical stride

    RK_U16      endian_mode;     // for BPP
    RK_U16      alpha_swap;
} RgaImg;

typedef struct RgaRect_t {
    RK_U16      xmin;
    RK_U16      xmax;
    RK_U16      ymin;
    RK_U16      ymax;
} RgaRect;

typedef struct RgaPoint_t {
    RK_U16      x;
    RK_U16      y;
} RgaPoint;

typedef struct RgaColorFill_t {
    RK_S16      gr_x_a;
    RK_S16      gr_y_a;
    RK_S16      gr_x_b;
    RK_S16      gr_y_b;
    RK_S16      gr_x_g;
    RK_S16      gr_y_g;
    RK_S16      gr_x_r;
    RK_S16      gr_y_r;
} RgaColorFill;

typedef struct RgaLineDraw_t {
    RgaPoint    start_point;
    RgaPoint    end_point;
    RK_U32      color;
    RK_U32      flag;
    RK_U32      line_width;
} RgaLineDraw;

typedef struct RgaFading_t {
    RK_U8       b;
    RK_U8       g;
    RK_U8       r;
    RK_U8       res;
} RgaFading;

typedef struct RgaMmu_t {
    RK_U8       mmu_en;
    RK_ULONG    base_addr;
    RK_U32      mmu_flag;
} RgaMmu;

// structure for userspace / kernel communication
typedef struct RgaRequest_t {
    RK_U8       render_mode;

    RgaImg      src;
    RgaImg      dst;
    RgaImg      pat;

    RK_ULONG    rop_mask_addr;      /* rop4 mask addr */
    RK_ULONG    LUT_addr;           /* LUT addr */

    RgaRect     clip;               /* dst clip window default value is dst_vir */
    /* value from [0, w-1] / [0, h-1]*/

    RK_S32      sina;               /* dst angle  default value 0  16.16 scan from table */
    RK_S32      cosa;               /* dst angle  default value 0  16.16 scan from table */

    /* alpha rop process flag           */
    /* ([0] = 1 alpha_rop_enable)       */
    /* ([1] = 1 rop enable)             */
    /* ([2] = 1 fading_enable)          */
    /* ([3] = 1 PD_enable)              */
    /* ([4] = 1 alpha cal_mode_sel)     */
    /* ([5] = 1 dither_enable)          */
    /* ([6] = 1 gradient fill mode sel) */
    /* ([7] = 1 AA_enable)              */
    RK_U16      alpha_rop_flag;
    RK_U8       scale_mode;         /* 0 nearst / 1 bilnear / 2 bicubic */

    RK_U32      color_key_max;      /* color key max */
    RK_U32      color_key_min;      /* color key min */

    RK_U32      fg_color;           /* foreground color */
    RK_U32      bg_color;           /* background color */

    RgaColorFill gr_color;          /* color fill use gradient */

    RgaLineDraw line_draw_info;

    RgaFading   fading;

    RK_U8       PD_mode;            /* porter duff alpha mode sel */
    RK_U8       alpha_global_value; /* global alpha value */
    RK_U16      rop_code;           /* rop2/3/4 code  scan from rop code table*/
    RK_U8       bsfilter_flag;      /* [2] 0 blur 1 sharp / [1:0] filter_type*/
    RK_U8       palette_mode;       /* (enum) color palatte  0/1bpp, 1/2bpp 2/4bpp 3/8bpp */
    RK_U8       yuv2rgb_mode;       /* (enum) BT.601 MPEG / BT.601 JPEG / BT.709  */
    RK_U8       endian_mode;

    /* (enum) rotate mode  */
    /* 0x0,     no rotate  */
    /* 0x1,     rotate     */
    /* 0x2,     x_mirror   */
    /* 0x3,     y_mirror   */
    RK_U8       rotate_mode;

    RK_U8       color_fill_mode;    /* 0 solid color / 1 patten color */

    RgaMmu      mmu_info;           /* mmu information */

    /* ([0~1] alpha mode)       */
    /* ([2~3] rop   mode)       */
    /* ([4]   zero  mode en)    */
    /* ([5]   dst   alpha mode) */
    RK_U8       alpha_rop_mode;
    RK_U8       src_trans_mode;
    RK_U8       CMD_fin_int_enable;

    /* completion is reported through a callback */
    void (*complete)(int retval);
} RgaReq;

#endif // __MPP_RGA_H__
