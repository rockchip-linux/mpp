/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
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
        /* VPU_CLIENT_AV1DEC        */  "av1dec",
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
        /* VPU_CLIENT_VEPU2_JPEG    */  "vepu2_jpeg",
        /* VPU_CLIENT_JPEG_ENC      */  "rkjpege",
        /* 21 ~ 23 */
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
