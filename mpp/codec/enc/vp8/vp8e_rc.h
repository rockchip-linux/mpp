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

#ifndef __VP8E_RC_H__
#define __VP8E_RC_H__

#include "vp8e_syntax.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET vp8e_init_rc(Vp8eRc *rc, MppEncCfgSet *cfg);
MPP_RET vp8e_update_rc_cfg(Vp8eRc *rc, MppEncRcCfg *cfg);
MPP_RET vp8e_before_pic_rc(Vp8eRc *rc);
MPP_RET vp8e_after_pic_rc(Vp8eRc *rc, RK_S32 bitcnt);

#ifdef  __cplusplus
}
#endif


#endif //__VP8E_RC_H__
