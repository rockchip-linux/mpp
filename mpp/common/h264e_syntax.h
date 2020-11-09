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

#ifndef __H264E_SYNTAX_H__
#define __H264E_SYNTAX_H__

#include "mpp_rc_api.h"
#include "h264_syntax.h"

typedef enum H264eSyntaxType_e {
    H264E_SYN_CFG,
    H264E_SYN_SPS,
    H264E_SYN_PPS,
    H264E_SYN_DPB,
    H264E_SYN_SLICE,
    H264E_SYN_FRAME,
    H264E_SYN_PREFIX,
    H264E_SYN_RC_RET,
    H264E_SYN_BUTT,
} H264eSyntaxType;

#define SYN_TYPE_FLAG(type)         (1 << (type))

typedef struct H264eSyntaxDesc_t {
    H264eSyntaxType     type;
    void                *p;
} H264eSyntaxDesc;

typedef struct H264ePrefixNal_t {
    RK_S32      nal_ref_idc;

    /* svc extension header */
    RK_S32      idr_flag;
    RK_S32      priority_id;
    RK_S32      no_inter_layer_pred_flag;
    RK_S32      dependency_id;
    RK_S32      quality_id;
    RK_S32      temporal_id;
    RK_S32      use_ref_base_pic_flag;
    RK_S32      discardable_flag;
    RK_S32      output_flag;
} H264ePrefixNal;

typedef struct H264eReorderInfo_t   H264eReorderInfo;
typedef struct H264eMarkingInfo_t   H264eMarkingInfo;
typedef struct H264eDpbFrm_t        H264eDpbFrm;

/*
 * Split reference frame configure to two parts
 * The first part is slice depended info like poc / frame_num, and frame
 * type and flags.
 * The other part is gop structure depended info like gop index, ref_status
 * and ref_frm_index. This part is inited from dpb gop hierarchy info.
 */
typedef struct H264eFrmInfo_s {
    RK_S32      seq_idx;

    RK_S32      curr_idx;
    RK_S32      refr_idx;

    RK_S32      usage[H264E_MAX_REFS_CNT + 1];
} H264eFrmInfo;

/* macro to add syntax to description array */
#define H264E_ADD_SYNTAX(desc, number, syn_type, syn_ptr) \
    do { \
        desc[number].type  = syn_type; \
        desc[number].p     = syn_ptr; \
        number++; \
    } while (0)

#endif /* __H264E_SYNTAX_H__ */
