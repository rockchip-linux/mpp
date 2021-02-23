/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_2str.h"

#include "hal_info.h"

typedef struct HalInfoImpl_t {
    MppCtxType      type;
    MppCodingType   coding;

    RK_U32          updated;
    RK_U32          elem_nb;
    /* info data for output */
    MppDevInfoCfg   *elems;
} HalInfoImpl;

MPP_RET hal_info_init(HalInfo *ctx, MppCtxType type, MppCodingType coding)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_NOK;
    RK_U32 elem_nb = (type == MPP_CTX_DEC) ? (ENC_INFO_BUTT) : (DEC_INFO_BUTT);
    HalInfoImpl *impl = mpp_calloc_size(HalInfoImpl, sizeof(HalInfoImpl) +
                                        sizeof(MppDevInfoCfg) * elem_nb);
    if (impl) {
        impl->type = type;
        impl->coding = coding;
        impl->elem_nb = elem_nb;
        impl->elems = (MppDevInfoCfg *)(impl + 1);
        ret = MPP_OK;
    }

    *ctx = impl;

    return ret;
}

MPP_RET hal_info_deinit(HalInfo ctx)
{
    MPP_FREE(ctx);
    return MPP_OK;
}

MPP_RET hal_info_set(HalInfo ctx, RK_U32 type, RK_U32 flag, RK_U64 data)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    if (flag <= CODEC_INFO_FLAG_NULL || flag >= CODEC_INFO_FLAG_BUTT) {
        mpp_err_f("found invalid flag %d\n", flag);
        return MPP_ERR_VALUE;
    }

    HalInfoImpl *info = (HalInfoImpl *)ctx;
    MppDevInfoCfg *elems = NULL;

    switch (info->type) {
    case MPP_CTX_DEC : {
        if (type <= DEC_INFO_BASE || type >= DEC_INFO_BUTT) {
            mpp_err_f("found invalid dec info type %d [%d:%d]\n",
                      type, DEC_INFO_BASE, DEC_INFO_BUTT);
            return MPP_ERR_VALUE;
        }

        /* shift enum base */
        type -= DEC_INFO_BASE;
    } break;
    case MPP_CTX_ENC : {
        if (type <= ENC_INFO_BASE || type >= ENC_INFO_BUTT) {
            mpp_err_f("found invalid enc info type %d [%d:%d]\n",
                      type, ENC_INFO_BASE, ENC_INFO_BUTT);
            return MPP_ERR_VALUE;
        }

        /* shift enum base */
        type -= ENC_INFO_BASE;
    } break;
    default : {
        mpp_err_f("found invalid ctx type %d\n", info->type);
        return MPP_ERR_VALUE;
    } break;
    }

    elems = &info->elems[type];

    if (elems->type != type || elems->flag != flag || elems->data != data) {
        /* set enc info */
        elems->type = type;
        elems->flag = flag;
        elems->data = data;
        info->updated |= (1 << type);
    }

    return MPP_OK;
}

MPP_RET hal_info_get(HalInfo ctx, MppDevInfoCfg *data, RK_S32 *size)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    if (NULL == data || NULL == size || *size == 0) {
        mpp_err_f("found invalid output cfg data %p size %p\n", data, size);
        return MPP_ERR_NULL_PTR;
    }

    HalInfoImpl *info = (HalInfoImpl *)ctx;
    if (!info->updated) {
        *size = 0;
        return MPP_OK;
    }

    RK_S32 max_size = *size;
    RK_S32 elem_size = sizeof(info->elems[0]);
    RK_S32 out_size = 0;
    RK_S32 type_max = 0;
    RK_S32 i;

    switch (info->type) {
    case MPP_CTX_DEC : {
        type_max = DEC_INFO_BUTT - DEC_INFO_BASE;
    } break;
    case MPP_CTX_ENC : {
        type_max = ENC_INFO_BUTT - ENC_INFO_BASE;
    } break;
    default : {
        mpp_err_f("found invalid ctx type %d\n", info->type);
        return MPP_ERR_VALUE;
    } break;
    }

    for (i = 0; i < type_max; i++) {
        if (!(info->updated & (1 << i)))
            continue;

        if (out_size + elem_size > max_size) {
            mpp_err_f("out data size %d is too small for %d\n",
                      max_size, out_size + elem_size);
            break;
        }

        memcpy(data, &info->elems[i], elem_size);
        data++;
        out_size += elem_size;
        info->updated &= ~(1 << i);
    }

    *size = out_size;
    return MPP_OK;
}

RK_U64 hal_info_to_string(HalInfo ctx, RK_U32 type, void *val)
{
    RK_U64 ret = 0;

    if (NULL == ctx || NULL == val) {
        mpp_err_f("found NULL input ctx %p val %p\n", ctx, val);
        return ret;
    }

    HalInfoImpl *info = (HalInfoImpl *)ctx;
    const char *str = NULL;

    switch (info->type) {
    case MPP_CTX_DEC : {
        switch (type) {
        case DEC_INFO_FORMAT : {
            MppCodingType coding = *((MppCodingType *)val);

            mpp_assert(coding == info->coding);
            str = strof_coding_type(coding);
        } break;
        default : {
        } break;
        }
    } break;
    case MPP_CTX_ENC : {
        switch (type) {
        case ENC_INFO_FORMAT : {
            MppCodingType coding = *((MppCodingType *)val);

            mpp_assert(coding == info->coding);
            str = strof_coding_type(coding);
        } break;
        case ENC_INFO_RC_MODE : {
            MppEncRcMode rc_mode = *((MppEncRcMode *)val);

            str = strof_rc_mode(rc_mode);
        } break;
        case ENC_INFO_PROFILE : {
            RK_U32 profile = *((RK_U32 *)val);

            str = strof_profle(info->coding, profile);
        } break;
        default : {
        } break;
        }
    } break;
    default : {
        mpp_err_f("found invalid ctx type %d\n", info->type);
        return MPP_ERR_VALUE;
    } break;
    }

    if (str)
        snprintf((void *)&ret, sizeof(ret) - 1, "%s", str);

    return ret;
}

RK_U64 hal_info_to_float(RK_S32 num, RK_S32 denorm)
{
    RK_U64 ret = 0;

    if (!denorm)
        snprintf((void *)&ret, sizeof(ret) - 1, "%d", num);
    else
        snprintf((void *)&ret, sizeof(ret) - 1, "%.2f", (float)num / denorm);

    return ret;
}

MPP_RET hal_info_from_enc_cfg(HalInfo ctx, MppEncCfgSet *cfg)
{
    MppEncRcCfg *rc = &cfg->rc;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncCodecCfg *codec = &cfg->codec;
    HalInfoImpl *info = (HalInfoImpl *)ctx;
    RK_U32 profile = 0;
    RK_U64 val = 0;

    hal_info_set(ctx, ENC_INFO_WIDTH, CODEC_INFO_FLAG_NUMBER, prep->width);
    hal_info_set(ctx, ENC_INFO_HEIGHT, CODEC_INFO_FLAG_NUMBER, prep->height);

    val = hal_info_to_string(ctx, ENC_INFO_FORMAT, &info->coding);

    hal_info_set(ctx, ENC_INFO_FORMAT, CODEC_INFO_FLAG_STRING, val);
    hal_info_set(ctx, ENC_INFO_FPS_IN, CODEC_INFO_FLAG_NUMBER,
                 rc->fps_in_num / rc->fps_in_denorm);
    hal_info_set(ctx, ENC_INFO_FPS_OUT, CODEC_INFO_FLAG_NUMBER,
                 rc->fps_out_num / rc->fps_out_denorm);

    val = hal_info_to_string(ctx, ENC_INFO_RC_MODE, &rc->rc_mode);
    hal_info_set(ctx, ENC_INFO_RC_MODE, CODEC_INFO_FLAG_STRING, val);

    hal_info_set(ctx, ENC_INFO_BITRATE, CODEC_INFO_FLAG_NUMBER, rc->bps_target);
    hal_info_set(ctx, ENC_INFO_GOP_SIZE, CODEC_INFO_FLAG_NUMBER, rc->gop);

    switch (info->coding) {
    case MPP_VIDEO_CodingAVC : {
        profile = codec->h264.profile;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        profile = codec->h265.profile;
    } break;
    case MPP_VIDEO_CodingMJPEG :
    case MPP_VIDEO_CodingVP8 :
    default : {
    } break;
    }
    val = hal_info_to_string(ctx, ENC_INFO_PROFILE, &profile);
    hal_info_set(ctx, ENC_INFO_PROFILE, CODEC_INFO_FLAG_STRING, val);

    return MPP_OK;
}
