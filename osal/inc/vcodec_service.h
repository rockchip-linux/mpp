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

#ifndef __VCODEC_SERVICE_H__
#define __VCODEC_SERVICE_H__

#include "rk_type.h"

#define EXTRA_INFO_MAGIC                    (0x4C4A46)

#define VPU_IOC_MAGIC                       'l'

#define VPU_IOC_SET_CLIENT_TYPE             _IOW(VPU_IOC_MAGIC, 1, unsigned long)
#define VPU_IOC_GET_HW_FUSE_STATUS          _IOW(VPU_IOC_MAGIC, 2, unsigned long)
#define VPU_IOC_SET_REG                     _IOW(VPU_IOC_MAGIC, 3, unsigned long)
#define VPU_IOC_GET_REG                     _IOW(VPU_IOC_MAGIC, 4, unsigned long)

#define VPU_IOC_SET_CLIENT_TYPE_U32         _IOW(VPU_IOC_MAGIC, 1, unsigned int)

#define VPU_IOC_WRITE(nr, size)             _IOC(_IOC_WRITE, VPU_IOC_MAGIC, (nr), (size))

#define VDPU1_REGISTERS                     (101)
#define VDPU2_REGISTERS                     (159)
#define VDPU1_PP_REGISTERS                  (164)
#define VDPU2_PP_REGISTERS                  (184)
#define RKHEVC_REGISTERS                    (68)
#define RKVDEC_REGISTERS                    (78)
#define AVSD_REGISTERS                      (60)

#define VEPU1_REGISTERS                     (164)
#define VEPU2_REGISTERS                     (184)
#define RKVENC_REGISTERS                    (140)

#define EXTRA_INFO_SIZE                     (sizeof(RK_U32) * 34)

#ifdef __cplusplus
extern "C" {
#endif

const char *mpp_get_vcodec_dev_name(MppCtxType type, MppCodingType coding);

#ifdef __cplusplus
}
#endif

#endif /* __VCODEC_SERVICE_H__ */
