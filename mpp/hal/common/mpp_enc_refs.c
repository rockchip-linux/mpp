/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_enc_refs"

#include <ctype.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp_enc_refs.h"

MPP_RET mpp_enc_get_hier_info(MppEncFrmRefInfo **info, MppEncHierCfg *cfg)
{
    RK_S32 i;
    RK_S32 length = cfg->length + 1;
    const char *fmt = cfg->ref_fmt;
    MPP_RET ret = MPP_OK;
    MppEncFrmRefInfo *p = mpp_calloc(MppEncFrmRefInfo, length);

    *info = NULL;

    if (NULL == p)
        return MPP_ERR_MALLOC;

    for (i = 0; i < length; i++) {
        MppEncFrmRefInfo *tmp = &p[i];

        /* parse frame type */
        switch (fmt[0]) {
        case 'I' : {
            tmp->status.is_intra = 1;
            tmp->status.is_idr = 1;
        } break;
        case 'i' : {
            tmp->status.is_intra = 1;
        } break;
        case 'P' :
        case 'p' : {
        } break;
        case 'B' :
        case 'b' :
        default : {
            mpp_err("invalid frame type %c\n", fmt[0]);
            ret = MPP_NOK;
            goto RET;
        } break;
        }

        /* First frame should be IDR frame */
        if (i == 0) {
            tmp->status.is_intra = 1;
            tmp->status.is_idr = 1;
        }
        fmt++;

        /* parse display order */
        if (isdigit(fmt[0])) {
            RK_S32 value = -1;

            sscanf(fmt, "%d", &value);
            mpp_assert(i == value);
        } else {
            mpp_err("invalid display order %c\n", fmt[0]);
            ret = MPP_NOK;
            goto RET;
        }
        fmt++;

        /* parse reference type */
        switch (fmt[0]) {
        case 'N' :
        case 'n' : {
            tmp->status.is_non_ref = 1;
        } break;
        case 'S' :
        case 's' : {
        } break;
        case 'L' :
        case 'l' : {
            tmp->status.is_lt_ref = 1;
            if (isdigit(fmt[1])) {
                tmp->status.lt_idx = (RK_U32)fmt[1] - 0x30;
                mpp_log("lt_idx %d\n", tmp->status.lt_idx);
                fmt++;
            }
        } break;
        default : {
            mpp_err("invalid reference type %c\n", fmt[0]);
            ret = MPP_NOK;
            goto RET;
        } break;
        }
        fmt++;

        /* parse temporal layer id */
        if (fmt[0] == 'T' || fmt[0] == 't') {
            fmt++;
            if (isdigit(fmt[0])) {
                RK_S32 value = -1;

                sscanf(fmt, "%d", &value);
                tmp->status.temporal_id = value;
            } else {
                mpp_err("invalid display order %c\n", fmt[0]);
                ret = MPP_NOK;
                goto RET;
            }
        } else {
            mpp_err("invalid temporal layer id flag %c\n", fmt[0]);
            ret = MPP_NOK;
            goto RET;
        }
        fmt++;

        tmp->ref_gop_idx = cfg->ref_idx[i];

        mpp_log_f("gop %d intra %d idr %d ref %d lt %d %d ref_idx %d\n",
                  i, tmp->status.is_intra, tmp->status.is_idr,
                  !tmp->status.is_non_ref, tmp->status.is_lt_ref,
                  tmp->status.lt_idx, tmp->ref_gop_idx);
    }

    *info = p;

RET:
    return ret;
}

MPP_RET mpp_enc_put_hier_info(MppEncFrmRefInfo *info)
{
    MPP_FREE(info);
    return MPP_OK;
}

