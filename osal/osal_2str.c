/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#include "osal_2str.h"

const char *strof_client_type(MppClientType type)
{
    static const char *client_type_name[] = {
        /* 0 ~ 3 */
        /* VPU_CLIENT_VDPU1         */  "vdpu1",
        /* VPU_CLIENT_VDPU2         */  "vdpu2",
        /* VPU_CLIENT_VDPU1_PP      */  "vdpu1_pp",
        /* VPU_CLIENT_VDPU2_PP      */  "vdpu2_pp",
        /* 4 ~ 7 */
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* 8 ~ 11 */
        /* VPU_CLIENT_HEVC_DEC      */  "rkhevc",
        /* VPU_CLIENT_RKVDEC        */  "rkvdec",
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* 12 ~ 15 */
        /* VPU_CLIENT_AVSPLUS_DEC   */  "avsd",
        /* VPU_CLIENT_JPEG_DEC      */  "rkjpegd",
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* 16 ~ 19 */
        /* VPU_CLIENT_RKVENC        */  "rkvenc",
        /* VPU_CLIENT_VEPU1         */  "vepu1",
        /* VPU_CLIENT_VEPU2         */  "vepu2",
        /* VPU_CLIENT_VEPU2_LITE    */  "vepu2_lite",
        /* 20 ~ 23 */
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* 24 ~ 27 */
        /* VPU_CLIENT_VEPU22        */  "vepu22",
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* 28 ~ 31 */
        /* IEP_CLIENT_TYPE          */  "iep",
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
        /* VPU_CLIENT_BUTT          */  NULL,
    };

    if (type < 0 || type >= VPU_CLIENT_BUTT)
        return NULL;

    return client_type_name[type];
}
