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

#ifndef __H265E_SYNTAX_H__
#define __H265E_SYNTAX_H__

typedef struct H265eSyntax_t {
    RK_S32          idr_request;
//   RK_S32          eos;
} H265eSyntax;

typedef struct H265eFeedback_t {
    RK_U32 bs_size;
    RK_U32 enc_pic_cnt;
    RK_U32 pic_type;
    RK_U32 avg_ctu_qp;
    RK_U32 gop_idx;
    RK_U32 poc;
    RK_U32 src_idx;
    RK_U32 status;
} H265eFeedback;

#endif
