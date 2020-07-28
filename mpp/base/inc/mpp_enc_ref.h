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

#ifndef __MPP_ENC_REF_H__
#define __MPP_ENC_REF_H__

#include "rk_venc_ref.h"

#define REF_MODE_IS_GLOBAL(mode)    ((mode >= REF_MODE_GLOBAL) && (mode < REF_MODE_GLOBAL_BUTT))
#define REF_MODE_IS_LT_MODE(mode)   ((mode > REF_MODE_LT) && (mode < REF_MODE_LT_BUTT))
#define REF_MODE_IS_ST_MODE(mode)   ((mode > REF_MODE_ST) && (mode < REF_MODE_ST_BUTT))

typedef struct MppEncCpbInfo_t {
    RK_S32              dpb_size;
    RK_S32              max_lt_cnt;
    RK_S32              max_st_cnt;
    RK_S32              max_lt_idx;
    RK_S32              max_st_tid;
    /* loop length of st/lt config */
    RK_S32              lt_gop;
    RK_S32              st_gop;
} MppEncCpbInfo;

typedef struct MppEncRefCfgImpl_t {
    const char          *name;
    RK_S32              ready;
    RK_U32              debug;

    /* config from user */
    RK_S32              keep_cpb;
    RK_S32              max_lt_cfg;
    RK_S32              max_st_cfg;
    RK_S32              lt_cfg_cnt;
    RK_S32              st_cfg_cnt;
    MppEncRefLtFrmCfg   *lt_cfg;
    MppEncRefStFrmCfg   *st_cfg;

    /* generated parameter for MppEncRefs */
    MppEncCpbInfo       cpb_info;
} MppEncRefCfgImpl;

#ifdef __cplusplus
extern "C" {
#endif

MppEncRefCfg mpp_enc_ref_default(void);
MPP_RET mpp_enc_ref_cfg_copy(MppEncRefCfg dst, MppEncRefCfg src);
MppEncCpbInfo *mpp_enc_ref_cfg_get_cpb_info(MppEncRefCfg ref);

#define check_is_mpp_enc_ref_cfg(ref) _check_is_mpp_enc_ref_cfg(__FUNCTION__, ref)
MPP_RET _check_is_mpp_enc_ref_cfg(const char *func, void *ref);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_REF_H__*/
