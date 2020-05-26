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

#define MODULE_TAG "mpp_enc_ref_test"

#include <string.h>

#include "mpp_log.h"

#include "rk_venc_ref.h"

int main()
{
    MPP_RET ret = MPP_OK;
    RK_S32 lt_cnt = 0;
    RK_S32 st_cnt = 0;
    MppEncRefCfg ref = NULL;
    MppEncRefLtFrmCfg lt_ref[4];
    MppEncRefStFrmCfg st_ref[16];

    memset(&lt_ref, 0, sizeof(lt_ref));
    memset(&st_ref, 0, sizeof(st_ref));

    mpp_log("mpp_enc_ref_test start\n");

    ret = mpp_enc_ref_cfg_init(&ref);

    mpp_log("mpp_enc_ref_test tsvc4 ref info generation start\n");

    lt_cnt = 1;
    st_cnt = 9;

    ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cnt, st_cnt);

    /* set 8 frame lt-ref gap */
    lt_ref[0].lt_idx        = 0;
    lt_ref[0].temporal_id   = 0;
    lt_ref[0].ref_mode      = REF_TO_PREV_LT_REF;
    lt_ref[0].lt_gap        = 8;
    lt_ref[0].lt_delay      = 0;

    ret = mpp_enc_ref_cfg_add_lt_cfg(ref, 1, lt_ref);

    /* set tsvc4 st-ref struct */
    /* st 0 layer 0 - ref */
    st_ref[0].is_non_ref    = 0;
    st_ref[0].temporal_id   = 0;
    st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
    st_ref[0].ref_arg       = 0;
    st_ref[0].repeat        = 0;
    /* st 1 layer 3 - non-ref */
    st_ref[1].is_non_ref    = 1;
    st_ref[1].temporal_id   = 3;
    st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[1].ref_arg       = 0;
    st_ref[1].repeat        = 0;
    /* st 2 layer 2 - ref */
    st_ref[2].is_non_ref    = 0;
    st_ref[2].temporal_id   = 2;
    st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[2].ref_arg       = 0;
    st_ref[2].repeat        = 0;
    /* st 3 layer 3 - non-ref */
    st_ref[3].is_non_ref    = 1;
    st_ref[3].temporal_id   = 3;
    st_ref[3].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[3].ref_arg       = 0;
    st_ref[3].repeat        = 0;
    /* st 4 layer 1 - ref */
    st_ref[4].is_non_ref    = 0;
    st_ref[4].temporal_id   = 1;
    st_ref[4].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[4].ref_arg       = 0;
    st_ref[4].repeat        = 0;
    /* st 5 layer 3 - non-ref */
    st_ref[5].is_non_ref    = 1;
    st_ref[5].temporal_id   = 3;
    st_ref[5].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[5].ref_arg       = 0;
    st_ref[5].repeat        = 0;
    /* st 6 layer 2 - ref */
    st_ref[6].is_non_ref    = 0;
    st_ref[6].temporal_id   = 2;
    st_ref[6].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[6].ref_arg       = 0;
    st_ref[6].repeat        = 0;
    /* st 7 layer 3 - non-ref */
    st_ref[7].is_non_ref    = 1;
    st_ref[7].temporal_id   = 3;
    st_ref[7].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[7].ref_arg       = 0;
    st_ref[7].repeat        = 0;
    /* st 8 layer 0 - ref */
    st_ref[8].is_non_ref    = 0;
    st_ref[8].temporal_id   = 0;
    st_ref[8].ref_mode      = REF_TO_TEMPORAL_LAYER;
    st_ref[8].ref_arg       = 0;
    st_ref[8].repeat        = 0;

    ret = mpp_enc_ref_cfg_add_st_cfg(ref, 9, st_ref);

    /* check and get dpb size */
    mpp_log("mpp_enc_ref_test tsvc4 ref info verification start\n");
    ret = mpp_enc_ref_cfg_check(ref);
    mpp_log("mpp_enc_ref_test tsvc4 ref info verification ret %d\n", ret);

    ret = mpp_enc_ref_cfg_show(ref);

    /* reset for next config */
    ret = mpp_enc_ref_cfg_reset(ref);

#if 0
    mpp_log("mpp_enc_ref_test smartp ref info generation start\n");

    /* typical smartp config */
    lt_cnt = 1;
    st_cnt = 3;

    ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cnt, st_cnt);

    memset(&lt_ref, 0, sizeof(lt_ref));
    memset(&st_ref, 0, sizeof(st_ref));

    /* set 300 frame lt-ref gap */
    lt_ref[0].lt_idx        = 0;
    lt_ref[0].temporal_id   = 0;
    lt_ref[0].ref_mode      = REF_TO_PREV_INTRA;
    lt_ref[0].lt_gap        = 300;
    lt_ref[0].lt_delay      = 0;

    ret = mpp_enc_ref_cfg_add_lt_cfg(ref, 1, lt_ref);

    st_ref[0].is_non_ref    = 0;
    st_ref[0].temporal_id   = 0;
    st_ref[0].ref_mode      = REF_TO_PREV_LT_REF;
    st_ref[0].ref_arg       = 0;
    st_ref[0].repeat        = 0;

    st_ref[1].is_non_ref    = 0;
    st_ref[1].temporal_id   = 0;
    st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
    st_ref[1].ref_arg       = 0;
    st_ref[1].repeat        = 299;

    st_ref[2].is_non_ref    = 0;
    st_ref[2].temporal_id   = 0;
    st_ref[2].ref_mode      = REF_TO_PREV_LT_REF;
    st_ref[2].ref_arg       = 0;
    st_ref[2].repeat        = 0;

    ret = mpp_enc_ref_cfg_add_st_cfg(ref, 3, st_ref);

    mpp_log("mpp_enc_ref_test smartp ref info verification start\n");
    ret = mpp_enc_ref_cfg_check(ref);
    mpp_log("mpp_enc_ref_test smartp ref info verification ret %d\n", ret);

    ret = mpp_enc_ref_cfg_deinit(&ref);
#endif

    mpp_log("mpp_enc_ref_test %s\n", ret ? "failed" : "success");

    return ret;
}
