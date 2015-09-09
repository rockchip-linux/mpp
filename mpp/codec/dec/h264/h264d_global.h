/*
*
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

#ifndef __H264D_GLOBAL_H__
#define __H264D_GLOBAL_H__

#include <stdio.h>
#include "rk_type.h"
#include "rk_mpi.h"


//!< define
#define MAXSPS                    32
#define MAXPPS                    256
#define START_PREFIX_3BYTE        3

#define MVC_INIT_VIEW_ID          -1
#define MAX_VIEW_NUM              1024
#define BASE_VIEW_IDX             0
#define MAX_NUM_DPB_LAYERS        2
#define MAX_DPB_SIZE              35
#define MAX_NUM_DECSLICES         16
#define MAX_LIST_SIZE             33
#define MB_BLOCK_SIZE             16

#define DPB_INFO_SIZE             16
#define REFPIC_INFO_SIZE          32
#define NALU_BUF_MAX_SIZE         64//512*1024
#define NALU_BUF_ADD_SIZE         32//512


//!< AVC Profile IDC definitions
typedef enum {
    FREXT_CAVLC444 = 44,       //!< YUV 4:4:4/14 "CAVLC 4:4:4"
    BASELINE       = 66,       //!< YUV 4:2:0/8  "Baseline"
    H264_MAIN      = 77,       //!< YUV 4:2:0/8  "Main"
    EXTENDED       = 88,       //!< YUV 4:2:0/8  "Extended"
    FREXT_HP       = 100,      //!< YUV 4:2:0/8  "High"
    FREXT_Hi10P    = 110,      //!< YUV 4:2:0/10 "High 10"
    FREXT_Hi422    = 122,      //!< YUV 4:2:2/10 "High 4:2:2"
    FREXT_Hi444    = 244,      //!< YUV 4:4:4/14 "High 4:4:4"
    MVC_HIGH       = 118,      //!< YUV 4:2:0/8  "Multiview High"
    STEREO_HIGH    = 128       //!< YUV 4:2:0/8  "Stereo High"
} ProfileIDC;

//!< values for nal_unit_type
typedef enum {
    NALU_TYPE_NULL      = 0,
    NALU_TYPE_SLICE     = 1,
    NALU_TYPE_DPA       = 2,
    NALU_TYPE_DPB       = 3,
    NALU_TYPE_DPC       = 4,
    NALU_TYPE_IDR       = 5,
    NALU_TYPE_SEI       = 6,
    NALU_TYPE_SPS       = 7,
    NALU_TYPE_PPS       = 8,
    NALU_TYPE_AUD       = 9,   // Access Unit Delimiter
    NALU_TYPE_EOSEQ     = 10,  // end of sequence
    NALU_TYPE_EOSTREAM  = 11,  // end of stream
    NALU_TYPE_FILL      = 12,
    NALU_TYPE_SPSEXT    = 13,
    NALU_TYPE_PREFIX    = 14,  // prefix
    NALU_TYPE_SUB_SPS   = 15,
    NALU_TYPE_SLICE_AUX = 19,
    NALU_TYPE_SLC_EXT   = 20,  // slice extensive
    NALU_TYPE_VDRD      = 24   // View and Dependency Representation Delimiter NAL Unit
} Nalu_type;

//!< values for nal_ref_idc
typedef enum {
    NALU_PRIORITY_HIGHEST    = 3,
    NALU_PRIORITY_HIGH       = 2,
    NALU_PRIORITY_LOW        = 1,
    NALU_PRIORITY_DISPOSABLE = 0
} NalRefIdc_type;












//!< input parameter
typedef struct h264d_input_ctx_t {
    RK_U8  is_eos;
    struct h264_dec_ctx_t      *p_Dec;
    struct h264d_cur_ctx_t     *p_Cur;   //!< current parameters, use in read nalu
    struct h264d_video_ctx_t   *p_Vid;   //!< parameters for video decoder
    enum   mpp_decmtd_type      dec_mtd;

    struct h264d_inctx_t   *p_in;
    struct h264d_outctx_t  *p_out;
} H264dInputCtx_t;

//!< current stream
typedef struct h264d_curstrm_t {
    RK_U32   offset;         //!< The offset of the input stream
    RK_U32   max_size;       //!< Cur Unit Buffer size
    RK_U8    *buf;           //!< store read nalu data
    RK_U8    prefixdata[START_PREFIX_3BYTE];
    RK_U8    startcode_found;
    RK_U8    endcode_found;
} H264dCurStream_t;

//!< current parameters
typedef struct h264d_cur_ctx_t {
    struct h264_sps_t        *sps;
    struct h264_subsps_t     *subsps;
    struct h264_pps_t        *pps;
    struct h264_sei_t        *sei;
    struct h264_nalu_t       *nalu;
    struct getbit_ctx_t      *bitctx; //!< for control bit_read
    struct h264d_curstrm_t   *strm;
    struct h264_slice_t      *slice;
    struct h264_store_pic_t **listP[2];
    struct h264_store_pic_t **listB[2];
    struct h264d_input_ctx_t *p_Inp;
    struct h264_dec_ctx_t    *p_Dec;
    struct h264d_video_ctx_t *p_Vid;   //!< parameters for video decoder
} H264dCurCtx_t;

//!< parameters for video decoder
typedef struct h264d_video_ctx_t {
    struct h264_sps_t            *spsSet;      //!< MAXSPS, all sps storage
    struct h264_subsps_t         *subspsSet;   //!< MAXSPS, all subpps storage
    struct h264_pps_t            *ppsSet;      //!< MAXPPS, all pps storage
    struct h264_sps_t            *active_sps;
    struct h264_subsps_t         *active_subsps;
    struct h264_pps_t            *active_pps;
    struct h264_dec_ctx_t        *p_Dec;  //!< H264_DecCtx_t
    struct h264d_input_ctx_t     *p_Inp;  //!< H264_InputParameters
    struct h264d_cur_ctx_t       *p_Cur;  //!< H264_CurParameters
    struct h264_dpb_buf_t        *p_Dpb_layer[MAX_NUM_DPB_LAYERS];
    struct h264_store_pic_t      *dec_picture;
    struct h264_store_pic_t      *no_reference_picture;
    struct h264_frame_store_t    *out_buffer;
    struct h264_dpb_mark_t       *active_dpb_mark[MAX_NUM_DPB_LAYERS];  //!< acitve_dpb_memory
    struct h264_slice_t          *ppSliceList[MAX_NUM_DECSLICES]; //!< H264_SLICE_t
    struct h264_old_slice_par_t  *old_slice;
    RK_S32    *qmatrix[12];  //!< scanlist pointer
    RK_U32    stream_size;
    RK_S32    last_toppoc[MAX_NUM_DPB_LAYERS];
    RK_S32    last_bottompoc[MAX_NUM_DPB_LAYERS];
    RK_S32    last_framepoc[MAX_NUM_DPB_LAYERS];
    RK_S32    last_thispoc[MAX_NUM_DPB_LAYERS];
    //!<
    RK_S32     profile_idc;                                       // u(8)
    RK_S32     slice_type;
    RK_S32     structure;
    RK_S32     iNumOfSlicesDecoded;
    RK_S32     no_output_of_prior_pics_flag;
    RK_S32     last_has_mmco_5;
    RK_S32     max_frame_num;
    RK_S32     active_sps_id[MAX_NUM_DPB_LAYERS];
    RK_U32     PicWidthInMbs;
    RK_U32     FrameHeightInMbs;
    RK_S32     yuv_format;
    RK_S32     width;
    RK_S32     height;
    RK_S32     width_cr;                               //!< width chroma
    RK_S32     height_cr;                              //!< height chroma
    RK_S32     last_pic_structure;
    RK_S32     last_pic_bottom_field;
    RK_S32     last_pic_width_in_mbs_minus1[2];
    RK_S32     last_pic_height_in_map_units_minus1[2];
    RK_S32     last_profile_idc[2];
    RK_S32     last_level_idc[2];
    RK_S32     PrevPicOrderCntMsb;
    RK_S32     PrevPicOrderCntLsb;
    RK_U32     PreviousFrameNum;
    RK_U32     FrameNumOffset;
    RK_S32     PreviousFrameNumOffset;
    RK_S32     ExpectedDeltaPerPicOrderCntCycle;
    RK_S32     ExpectedPicOrderCnt;
    RK_S32     PicOrderCntCycleCnt;
    RK_S32     FrameNumInPicOrderCntCycle;
    RK_S32     ThisPOC;
    RK_S32     type;
    RK_U32     dec_no;
    //!< for control running
    RK_S32     have_outpicture_flag;
    RK_S32     exit_picture_flag;
    RK_S32      active_mvc_sps_flag;

    RK_U32     g_framecnt;

} H264dVideoCtx_t;

//!< decoder video parameter
typedef struct h264_dec_ctx_t {
    struct h264_dpb_mark_t    *dpb_mark;         //!< for write out, MAX_DPB_SIZE
    struct h264_dpb_info_t    *dpb_info;         //!< 16
    struct h264_refpic_info_t *refpic_info_p;    //!< 32
    struct h264_refpic_info_t *refpic_info[2];   //!< [2][32]
    struct h264d_dxva_ctx_t   *dxva_ctx;
    RK_U32                     task_max_size;

    struct h264d_input_ctx_t  *p_Inp;
    struct h264d_cur_ctx_t    *p_Cur;            //!< current parameters, use in read nalu
    struct h264d_video_ctx_t  *p_Vid;            //!< parameters for video decoder
    RK_U32                     spt_decode_mtds;  //!< support decoder methods
    RK_S32                     nalu_ret;         //!< current nalu state
    RK_S32                     next_state;       //!< RKV_SLICE_STATUS
    RK_U8                      first_frame_flag;
    RK_U8                      parser_end_flag;
    struct h264d_logctx_t     *logctx;           //!< debug log file

} H264_DecCtx_t;



#endif /* __H264D_GLOBAL_H__ */