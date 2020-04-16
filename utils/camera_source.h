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

#ifndef __CAMERA_SOURCE_H__
#define __CAMERA_SOURCE_H__

#include "mpp_frame.h"

typedef struct CamSource CamSource;

#ifdef __cplusplus
extern "C" {
#endif

// Create a new context to capture frames from <fname>. Returns NULL on error.
CamSource* camera_source_init(const char *device, RK_U32 bufcnt, RK_U32 width, RK_U32 height, MppFrameFormat fmt);

// Stop capturing and free a context.
MPP_RET camera_source_deinit(CamSource *ctx);

// Returns the next captured frame and its meta-data.
RK_S32 camera_source_get_frame(CamSource *ctx);

// Tells the kernel it's OK to overwrite a frame captured
MPP_RET camera_source_put_frame(CamSource *ctx, RK_S32 idx);

MppBuffer camera_frame_to_buf(CamSource *ctx, RK_S32 idx);

#ifdef __cplusplus
}
#endif

#endif /* __CAMERA_SOURCE_H__ */
