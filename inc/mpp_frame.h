/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __MPP_FRAME_H__
#define __MPP_FRAME_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef void* MppFrame;


#ifdef __cplusplus
extern "C" {
#endif

/*
 * MppFrame interface
 */
MPP_RET mpp_frame_init(MppFrame *frame, RK_U8 *data, RK_U32 size);
MPP_RET mpp_frame_deinit(MppFrame *frame);
MPP_RET mpp_frame_get_info(MppFrame frame);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_FRAME_H__*/
