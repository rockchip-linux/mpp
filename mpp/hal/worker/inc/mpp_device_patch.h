/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_DEVICE_PATCH_H__
#define __MPP_DEVICE_PATCH_H__

#include "rk_type.h"

#define EXTRA_INFO_MAGIC    (0x4C4A46)

typedef struct RegPatchSlotInfo_t {
    RK_U32          reg_idx;
    RK_U32          offset;
} RegPatchInfo;

typedef struct RegExtraInfo_t {
    RK_U32          magic;      // Fix magic value 0x4C4A46
    RK_U32          count;      // valid patch info count
    RegPatchInfo    patchs[5];
} RegExtraInfo;

#ifdef __cplusplus
extern "C"
{
#endif

/* Reset RegExtraInfo structure magic and reset count to zero */
void mpp_device_patch_init(RegExtraInfo *extra);

/*
 * Register file has hardware address register which value is composed by:
 * bit  0 -  9  - ion/drm buffer fd
 * bit 10 - 31  - offset from the buffer start address
 * When the offset is larger then 4M (22 bit) the 32-bit register can not
 * save all the bits use a extra info structure right behind register file array
 * Then the extra info structure is RegExtraInfo.
 */
void mpp_device_patch_add(RK_U32 *reg, RegExtraInfo *extra, RK_U32 reg_idx,
                          RK_U32 offset);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_DEVICE_PATCH_H__ */
