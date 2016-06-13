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

#ifndef __MPP_INFO_H__
#define __MPP_INFO_H__

typedef enum RK_CHIP_TYPE {
    NONE,
    RK29,
    RK30,
    RK31,

    RK_CHIP_NUM = 0x100,
} RK_CHIP_TYPE;

typedef enum MPP_INFO_TYPE {
    INFO_ALL,
    INFO_REVISION,
    INFO_DATE,
    INFO_AUTHOR,

    INFO_BUTT,
} MPP_INFO_TYPE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

RK_CHIP_TYPE get_chip_type();
const char *mpp_info_get(MPP_INFO_TYPE type);
int mpp_info_get_revision();

#ifdef __cplusplus
}
#endif

#endif /*__MPP_INFO_H__*/
