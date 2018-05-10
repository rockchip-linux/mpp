/*
 * Copyright 2018 Rockchip Electronics Co. LTD
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

#ifndef __MPP_DEC_VPROC_H__
#define __MPP_DEC_VPROC_H__

typedef void* MppDecVprocCtx;

#ifdef __cplusplus
#include "mpp.h"

extern "C" {
#endif

/*
 * Functions usage:
 * dec_vproc_init   - get context with cfg
 * dec_vproc_deinit - stop thread and destory context
 * dec_vproc_start  - start thread processing
 * dec_vproc_signal - signal thread that one frame has be pushed for process
 * dec_vproc_reset  - reset process thread and discard all input
 */

MPP_RET dec_vproc_init(MppDecVprocCtx *ctx, void *mpp);
MPP_RET dec_vproc_deinit(MppDecVprocCtx ctx);

MPP_RET dec_vproc_start(MppDecVprocCtx ctx);
MPP_RET dec_vproc_stop(MppDecVprocCtx ctx);
MPP_RET dec_vproc_signal(MppDecVprocCtx ctx);
MPP_RET dec_vproc_reset(MppDecVprocCtx ctx);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_DEC_VPROC_H__ */
