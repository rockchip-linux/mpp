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

#ifndef __RK_VENC_RC_H__
#define __RK_VENC_RC_H__

#include "rk_type.h"

/* Rate control parameter */
typedef enum MppEncRcMode_e {
    MPP_ENC_RC_MODE_VBR,
    MPP_ENC_RC_MODE_CBR,
    MPP_ENC_RC_MODE_FIXQP,
    MPP_ENC_RC_MODE_AVBR,
    MPP_ENC_RC_MODE_BUTT
} MppEncRcMode;

typedef enum MppEncRcDropFrmMode_e {
    MPP_ENC_RC_DROP_FRM_DISABLED,
    MPP_ENC_RC_DROP_FRM_NORMAL,
    MPP_ENC_RC_DROP_FRM_PSKIP,
    MPP_ENC_RC_DROP_FRM_BUTT
} MppEncRcDropFrmMode;

#endif /*__RK_VENC_RC_H__*/
