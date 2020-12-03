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

#ifndef __RC_HAL_H__
#define __RC_HAL_H__

#include "mpp_platform.h"

#include "rc/rc_vepu.h"
#include "rc/rc_vepu540.h"

/* parameters to set to hardware */
typedef struct RcHalSet_t {
    /* unique sequential id */
    RK_S32              seq_id;
    RK_S32              target_bit;

    union {
        RcVepuSet       vepu;
        RcVepu540Set    vepu540;
    };
} RcHalSet;

/* parameters returned from hardware */
typedef struct RcHalRet_t {
    /* unique sequential id */
    RK_S32              seq_id;
    RK_S32              stream_size;

    union {
        RcVepuRet       vepu;
        RcVepu540Ret    vepu540;
    };
} RcHalRet;

#endif /* __RC_HAL_H__ */
