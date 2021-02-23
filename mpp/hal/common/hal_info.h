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

#ifndef __HAL_INFO_H__
#define __HAL_INFO_H__

#include "mpp_enc_cfg.h"
#include "mpp_device.h"

typedef enum CodecInfoType_e {
    /* ENC info */
    ENC_INFO_BASE           = 0,
    ENC_INFO_WIDTH          = (ENC_INFO_BASE + 1),
    ENC_INFO_HEIGHT         = (ENC_INFO_BASE + 2),
    ENC_INFO_FORMAT         = (ENC_INFO_BASE + 3),
    ENC_INFO_FPS_IN         = (ENC_INFO_BASE + 4),
    ENC_INFO_FPS_OUT        = (ENC_INFO_BASE + 5),
    ENC_INFO_RC_MODE        = (ENC_INFO_BASE + 6),
    ENC_INFO_BITRATE        = (ENC_INFO_BASE + 7),
    ENC_INFO_GOP_SIZE       = (ENC_INFO_BASE + 8),
    ENC_INFO_FPS_CALC       = (ENC_INFO_BASE + 9),
    ENC_INFO_PROFILE        = (ENC_INFO_BASE + 10),
    ENC_INFO_BUTT,

    /* DEC info */
    DEC_INFO_BASE           = 16,
    DEC_INFO_WIDTH          = (DEC_INFO_BASE + 1),
    DEC_INFO_HEIGHT         = (DEC_INFO_BASE + 2),
    DEC_INFO_FORMAT         = (DEC_INFO_BASE + 3),
    DEC_INFO_BITDEPTH       = (DEC_INFO_BASE + 4),
    DEC_INFO_FPS            = (DEC_INFO_BASE + 5),
    DEC_INFO_BUTT,
} CodecInfoType;

enum CODEC_INFO_FLAGS {
    CODEC_INFO_FLAG_NULL      = 0,
    CODEC_INFO_FLAG_NUMBER    = 1,
    CODEC_INFO_FLAG_STRING    = 2,
    CODEC_INFO_FLAG_BUTT,
};

typedef void* HalInfo;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET hal_info_init(HalInfo *ctx, MppCtxType type, MppCodingType coding);
MPP_RET hal_info_deinit(HalInfo ctx);
MPP_RET hal_info_set(HalInfo ctx, RK_U32 type, RK_U32 flag, RK_U64 data);
MPP_RET hal_info_get(HalInfo ctx, MppDevInfoCfg *data, RK_S32 *size);

RK_U64 hal_info_to_string(HalInfo ctx, RK_U32 type, void *val);
RK_U64 hal_info_to_float(RK_S32 num, RK_S32 denorm);

MPP_RET hal_info_from_enc_cfg(HalInfo ctx, MppEncCfgSet *cfg);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_INFO_H__ */
