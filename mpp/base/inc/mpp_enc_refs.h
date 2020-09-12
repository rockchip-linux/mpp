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

#ifndef __MPP_ENC_REFS_H__
#define __MPP_ENC_REFS_H__

#include "mpp_enc_cfg.h"
#include "mpp_enc_ref.h"

/*
 * MppEncRefs has two runtime mode:
 *
 * 1. dryrun mode
 * This mode is for estimating the dpb size and configure check.
 *
 * 2. runtime mode
 * Thie mode is for real dpb loop in real encoder workflow.
 *
 * When encoder is running user can change the frame property by MppEncRefFrmUsrCfg.
 */
#define ENC_FORCE_IDR           (0x00000001)
#define ENC_FORCE_PSKIP         (0x00000002)
#define ENC_FORCE_NONREF        (0x00000004)
#define ENC_FORCE_LT_REF_IDX    (0x00000008)
#define ENC_FORCE_TEMPORAL_ID   (0x00000010)
#define ENC_FORCE_REF_MODE      (0x00000020)

typedef struct MppEncRefFrmForceCfg_t {
    RK_U32              force_flag;
    RK_S32              force_idr;
    RK_S32              force_pskip;
    RK_S32              force_nonref;
    RK_S32              force_lt_idx;
    RK_S32              force_temporal_id;
    MppEncRefMode       force_ref_mode;
    RK_S32              force_ref_arg;
} MppEncRefFrmUsrCfg;

typedef void* MppEncRefs;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_enc_refs_init(MppEncRefs *refs);
MPP_RET mpp_enc_refs_deinit(MppEncRefs *refs);

MPP_RET mpp_enc_refs_set_cfg(MppEncRefs refs, MppEncRefCfg ref_cfg);
MPP_RET mpp_enc_refs_set_usr_cfg(MppEncRefs refs, MppEncRefFrmUsrCfg *force);
MPP_RET mpp_enc_refs_set_rc_igop(MppEncRefs refs, RK_S32 igop);

/* return hdr need update or not */
RK_S32  mpp_enc_refs_update_hdr(MppEncRefs refs);

/* get dpb size */
MPP_RET mpp_enc_refs_get_cpb_info(MppEncRefs refs, MppEncCpbInfo *info);
/* get status for next frame */
MPP_RET mpp_enc_refs_get_cpb(MppEncRefs refs, EncCpbStatus *status);
/* dryrun and check all configure */
MPP_RET mpp_enc_refs_dryrun(MppEncRefs refs);

MPP_RET mpp_enc_refs_stash(MppEncRefs refs);
MPP_RET mpp_enc_refs_rollback(MppEncRefs refs);

#define dump_frm(frm)   _dump_frm(frm, __FUNCTION__, __LINE__)

void _dump_frm(EncFrmStatus *frm, const char *func, RK_S32 line);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_REFS_H__*/
