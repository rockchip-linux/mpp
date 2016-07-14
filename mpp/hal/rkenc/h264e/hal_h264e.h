#ifndef __HAL_H264E_H__
#define __HAL_H264E_H__

#include "vpu.h"
#include "mpp_log.h"
#include "mpp_hal.h"
#include "h264e_syntax.h"

#define H264E_HAL_LOG_MODE              0x00001111

#define H264E_HAL_LOG_ERROR             0x00000001
#define H264E_HAL_LOG_ASSERT            0x00000010
#define H264E_HAL_LOG_WARNNING          0x00000100
#define H264E_HAL_LOG_FLOW              0x00001000
#define H264E_HAL_LOG_DETAIL            0x00010000
#define H264E_HAL_LOG_FILE              0x00100000


#define H264E_HAL_MASK_2b               (RK_U32)0x00000003
#define H264E_HAL_MASK_3b               (RK_U32)0x00000007
#define H264E_HAL_MASK_4b               (RK_U32)0x0000000F
#define H264E_HAL_MASK_5b               (RK_U32)0x0000001F
#define H264E_HAL_MASK_6b               (RK_U32)0x0000003F
#define H264E_HAL_MASK_11b              (RK_U32)0x000007FF
#define H264E_HAL_MASK_14b              (RK_U32)0x00003FFF
#define H264E_HAL_MASK_16b              (RK_U32)0x0000FFFF


#define h264e_hal_debug_enter() \
    do {\
        if (H264E_HAL_LOG_MODE & H264E_HAL_LOG_FLOW)\
            { mpp_log("line(%d), func(%s), enter", __LINE__, __FUNCTION__); }\
    } while (0)

#define h264e_hal_debug_leave() \
    do {\
        if (H264E_HAL_LOG_MODE & H264E_HAL_LOG_FLOW)\
            { mpp_log("line(%d), func(%s), leave", __LINE__, __FUNCTION__); }\
    } while (0)

#define h264e_hal_log_err(fmt, ...) \
            do {\
                if (H264E_HAL_LOG_MODE & H264E_HAL_LOG_ERROR)\
                    { mpp_err(fmt, ## __VA_ARGS__); }\
            } while (0)

#define h264e_hal_log_detail(fmt, ...) \
            do {\
                if (H264E_HAL_LOG_MODE & H264E_HAL_LOG_DETAIL)\
                    { mpp_log(fmt, ## __VA_ARGS__); }\
            } while (0)

#define H264E_HAL_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define H264E_HAL_MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define H264E_HAL_MIN3(a,b,c) H264E_HAL_MIN((a),H264E_HAL_MIN((b),(c)))
#define H264E_HAL_MAX3(a,b,c) H264E_HAL_MAX((a),H264E_HAL_MAX((b),(c)))
#define H264E_HAL_MIN4(a,b,c,d) H264E_HAL_MIN((a),H264E_HAL_MIN3((b),(c),(d)))
#define H264E_HAL_MAX4(a,b,c,d) H264E_HAL_MAX((a),H264E_HAL_MAX3((b),(c),(d)))
#define H264E_HAL_CLIP3(v, min, max)  ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

#define H264E_HAL_FCLOSE(fp)    do{ if(fp) fclose(fp); fp = NULL; } while (0)

#define H264E_HAL_SET_REG(reg, addr, val)                                    \
    do {                                                                     \
        reg[(addr)>>2] = (RK_U32)(val);                                      \
        if(H264E_HAL_LOG_MODE & 0/*H264E_HAL_LOG_INFO*/)                              \
            mpp_log("line(%d) set reg[%03d/%03x]: %08x", __LINE__, (addr)>>2, addr, val); \
    } while (0)


#define H264E_REF_MAX 16

typedef enum h264e_hal_slice_type_t {
    H264E_HAL_SLICE_TYPE_P  = 0,
    H264E_HAL_SLICE_TYPE_B  = 1,
    H264E_HAL_SLICE_TYPE_I  = 2,
} h264e_hal_slice_type;

typedef struct h264e_hal_sps_t {
    RK_S32 i_id;

    RK_S32 i_profile_idc;
    RK_S32 i_level_idc;

    RK_S32 b_constraint_set0;
    RK_S32 b_constraint_set1;
    RK_S32 b_constraint_set2;
    RK_S32 b_constraint_set3;

    RK_S32 i_log2_max_frame_num;

    RK_S32 i_poc_type;
    /* poc 0 */
    RK_S32 i_log2_max_poc_lsb;

    RK_S32 i_num_ref_frames;
    RK_S32 b_gaps_in_frame_num_value_allowed;
    RK_S32 i_mb_width;
    RK_S32 i_mb_height;
    RK_S32 b_frame_mbs_only;
    RK_S32 b_mb_adaptive_frame_field;
    RK_S32 b_direct8x8_inference;

    RK_S32 b_crop;
    struct {
        RK_S32 i_left;
        RK_S32 i_right;
        RK_S32 i_top;
        RK_S32 i_bottom;
    } crop;

    RK_S32 b_vui;
    struct {
        RK_S32 b_aspect_ratio_info_present;
        RK_S32 i_sar_width;
        RK_S32 i_sar_height;

        RK_S32 b_overscan_info_present;
        RK_S32 b_overscan_info;

        RK_S32 b_signal_type_present;
        RK_S32 i_vidformat;
        RK_S32 b_fullrange;
        RK_S32 b_color_description_present;
        RK_S32 i_colorprim;
        RK_S32 i_transfer;
        RK_S32 i_colmatrix;

        RK_S32 b_chroma_loc_info_present;
        RK_S32 i_chroma_loc_top;
        RK_S32 i_chroma_loc_bottom;

        RK_S32 b_timing_info_present;
        RK_U32 i_num_units_in_tick;
        RK_U32 i_time_scale;
        RK_S32 b_fixed_frame_rate;

        RK_S32 b_nal_hrd_parameters_present;
        RK_S32 b_vcl_hrd_parameters_present;

        struct {
            RK_S32 i_cpb_cnt;
            RK_S32 i_bit_rate_scale;
            RK_S32 i_cpb_size_scale;
            RK_S32 i_bit_rate_value;
            RK_S32 i_cpb_size_value;
            RK_S32 i_bit_rate_unscaled;
            RK_S32 i_cpb_size_unscaled;
            RK_S32 b_cbr_hrd;

            RK_S32 i_initial_cpb_removal_delay_length;
            RK_S32 i_cpb_removal_delay_length;
            RK_S32 i_dpb_output_delay_length;
            RK_S32 i_time_offset_length;
        } hrd;

        RK_S32 b_pic_struct_present;
        RK_S32 b_bitstream_restriction;
        RK_S32 b_motion_vectors_over_pic_boundaries;
        RK_S32 i_max_bytes_per_pic_denom;
        RK_S32 i_max_bits_per_mb_denom;
        RK_S32 i_log2_max_mv_length_horizontal;
        RK_S32 i_log2_max_mv_length_vertical;
        RK_S32 i_num_reorder_frames;
        RK_S32 i_max_dec_frame_buffering;

        /* FIXME to complete */
    } vui;

    RK_S32 b_qpprime_y_zero_transform_bypass;
    RK_S32 i_chroma_format_idc;
} h264e_hal_sps;

typedef struct h264e_hal_pps_t {
    RK_S32 i_id;
    RK_S32 i_sps_id;

    RK_S32 b_cabac;

    RK_S32 b_pic_order;
    RK_S32 i_num_slice_groups;

    RK_S32 i_num_ref_idx_l0_default_active;
    RK_S32 i_num_ref_idx_l1_default_active;

    RK_S32 b_weighted_pred;
    RK_S32 i_weighted_bipred_idc;

    RK_S32 i_pic_init_qp;
    RK_S32 i_pic_init_qs;

    RK_S32 i_chroma_qp_index_offset;
    RK_S32 i_second_chroma_qp_index_offset;

    RK_S32 b_deblocking_filter_control;
    RK_S32 b_constrained_intra_pred;
    RK_S32 b_redundant_pic_cnt;

    RK_S32 b_transform_8x8_mode;

    RK_S32 b_cqm_preset; //pic_scaling_list_present_flag
    const RK_U8 *scaling_list[8]; /* could be 12, but we don't allow separate Cb/Cr lists */
} h264e_hal_pps;

typedef enum h264e_rkv_nal_idx_t {
    RKV_H264E_RKV_NAL_IDX_SPS,
    RKV_H264E_RKV_NAL_IDX_PPS,
    RKV_H264E_RKV_NAL_IDX_BUTT,
} h264e_rkv_nal_idx;

typedef enum rkvenc_nal_unit_type_t {
    RKVENC_NAL_UNKNOWN     = 0,
    RKVENC_NAL_SLICE       = 1,
    RKVENC_NAL_SLICE_DPA   = 2,
    RKVENC_NAL_SLICE_DPB   = 3,
    RKVENC_NAL_SLICE_DPC   = 4,
    RKVENC_NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    RKVENC_NAL_SEI         = 6,    /* ref_idc == 0 */
    RKVENC_NAL_SPS         = 7,
    RKVENC_NAL_PPS         = 8,
    RKVENC_NAL_AUD         = 9,
    RKVENC_NAL_FILLER      = 12,
    /* ref_idc == 0 for 6,9,10,11,12 */
} rkvenc_nal_unit_type;

typedef enum rkvencnal_priority_t {
    RKVENC_NAL_PRIORITY_DISPOSABLE = 0,
    RKVENC_NAL_PRIORITY_LOW        = 1,
    RKVENC_NAL_PRIORITY_HIGH       = 2,
    RKVENC_NAL_PRIORITY_HIGHEST    = 3,
} rkvenc_nal_priority;

typedef struct h264e_hal_vui_param_t {
    /* they will be reduced to be 0 < x <= 65535 and prime */
    RK_S32         i_sar_height;
    RK_S32         i_sar_width;

    RK_S32         i_overscan;    /* 0=undef, 1=no overscan, 2=overscan */

    /* see h264 annex E for the values of the following */
    RK_S32         i_vidformat;
    RK_S32         b_fullrange;
    RK_S32         i_colorprim;
    RK_S32         i_transfer;
    RK_S32         i_colmatrix;
    RK_S32         i_chroma_loc;    /* both top & bottom */
} h264e_hal_vui_param;

typedef struct h264e_hal_ref_param_t {
    RK_S32         i_frame_reference;  /* Maximum number of reference frames */
    RK_S32         i_ref_pos;
    RK_S32         i_long_term_en;
    RK_S32         i_long_term_internal;
    RK_S32         hw_longterm_mode;
    RK_S32         i_dpb_size;         /* Force a DPB size larger than that implied by B-frames and reference frames.
                                        * Useful in combination with interactive error resilience. */
    RK_S32         i_frame_packing;
} h264e_hal_ref_param;


typedef struct h264e_hal_param_t {
    RK_S32 constrained_intra;

    h264e_hal_vui_param vui;
    h264e_hal_ref_param ref;

} h264e_hal_param;

typedef struct h264e_hal_context_t {
    MppHalApi           api;
    VPU_CLIENT_TYPE     vpu_client;
    RK_S32              vpu_socket;
    IOInterruptCB       int_cb;
    h264e_feedback      feedback;
    void                *regs;
    void                *ioctl_input;
    void                *ioctl_output;
    void                *buffers;
    void                *extra_info;
    void                *dpb_ctx;
    RK_U32              frame_cnt_gen_ready;
    RK_U32              frame_cnt_send_ready;
    RK_U32              num_frames_to_send;
    RK_U32              frame_cnt;
    h264e_hal_param     param;
    RK_U32              enc_mode;
    void                *dump_files;
} h264e_hal_context;

#endif
