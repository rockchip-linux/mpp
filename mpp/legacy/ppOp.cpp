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

#define LOG_TAG "PpOp"

#include "vpu.h"
#include "ppOp.h"

namespace android
{

status_t ppOpInit(PP_OP_HANDLE *hnd, PP_OPERATION *init)
{
    (void)hnd;
    (void)init;
    return 0;
}

status_t ppOpSet(PP_OP_HANDLE hnd, PP_SET_OPT opt, RK_U32 val)
{
    (void)hnd;
    (void)opt;
    (void)val;
    return 0;
}

status_t ppOpPerform(PP_OP_HANDLE hnd)
{
    (void)hnd;
    return 0;//VPU_REG_NUM_PP);
}

status_t ppOpSync(PP_OP_HANDLE hnd)
{
    (void)hnd;
    return 0;
}

status_t ppOpRelease(PP_OP_HANDLE hnd)
{
    (void)hnd;
    return 0;
}

}

#if BUILD_PPOP_TEST
#define SRC_WIDTH           (1920)//(1920)
#define SRC_HEIGHT          (1080)//(1080)
#define SRC_SIZE            (SRC_WIDTH*SRC_HEIGHT*2)

#define DST_WIDTH           (720)//(720)
#define DST_HEIGHT          (1280)
#define DST_SIZE            (DST_WIDTH*DST_HEIGHT*2)

#include "vpu_mem.h"
#include "vpu.h"
#include "ppOp.h"

int main()
{
    FILE *fpr, *fpw;
    int   wid_alig16 = ((SRC_WIDTH + 15) & (~0xf));
    int   hei_alig16 = ((SRC_HEIGHT + 15) & (~0xf));
    int   src_vir_width = 1920;//2048;
    int   src_vir_height = 1088;//1088;
    int   dst_vir_width = 800;//800;
    int   dst_vir_height = 1280;//1280;
    int   framecnt = 0;
    char *tmpbuf = (char *)malloc(src_vir_width * src_vir_height / 2);
    RK_S32 ret = 0, i,  j;

    ALOGI("ppOp test start\n");
    VPUMemLinear_t src, dst;
    android::PP_OP_HANDLE hnd;
    int vpuFd = VPUClientInit(VPU_PP);
    ret |= VPUMallocLinear(&src, src_vir_width * src_vir_height * 2); //SRC_SIZE);
    ret |= VPUMallocLinear(&dst, dst_vir_width * dst_vir_height * 2); //DST_SIZE);
    if (ret) {
        ALOGE("failed to malloc vpu_mem");
        goto end;
    }
    if (vpuFd < 0) {
        ALOGE("failed to open vpu client");
        goto end;
    }

    {
#if 0
        int i,  j;
        char *tmp = (char *)src.vir_addr;
        for (i = 0; i < SRC_HEIGHT; i++) {
            memset(tmp, (i & 0xff), SRC_WIDTH);
            tmp += SRC_WIDTH;
        }
#endif

#if 1
        char *tmp = (char *)src.vir_addr;
        fpr = fopen("/sdcard/testin.yuv", "rb");
        for (i = 0; i < SRC_HEIGHT; i++) {
            if (fpr)fread(tmp, 1, SRC_WIDTH, fpr);
            tmp += src_vir_width;
        }
        tmp = (char *)src.vir_addr;
        if (fpr)fread(&tmp[src_vir_width * src_vir_height], 1, SRC_WIDTH * SRC_HEIGHT / 2, fpr);
        if (fpr)fclose(fpr);

        for (i = 0; i < SRC_HEIGHT / 2; i++) { //planar to semi planar
            for (j = 0; j < SRC_WIDTH / 2; j++) { //planar to semi planar
                tmpbuf[i * src_vir_width + j * 2 + 0] = tmp[src_vir_width * src_vir_height + i * SRC_WIDTH / 2 + j];
                tmpbuf[i * src_vir_width + j * 2 + 1] = tmp[src_vir_width * src_vir_height + SRC_WIDTH * SRC_HEIGHT / 4 + i * SRC_WIDTH / 2 + j];
            }
        }
        memcpy(&tmp[src_vir_width * src_vir_height], tmpbuf, src_vir_width * src_vir_height / 2);
        //memset(&tmp[SRC_WIDTH*SRC_HEIGHT], 0x80, SRC_WIDTH*SRC_HEIGHT/2);
#endif
        VPUMemClean(&src);
    }

    while (1) {
        printf("framecnt=%d\n", framecnt);

        if (framecnt++ >= 1)
            break;

        wid_alig16 = ((SRC_WIDTH + 15) & (~0xf));
        hei_alig16 = ((SRC_HEIGHT + 15) & (~0xf));

        android::PP_OPERATION opt;
        memset(&opt, 0, sizeof(opt));
        opt.srcAddr     = src.phy_addr;
        opt.srcFormat   = PP_IN_FORMAT_YUV420SEMI;
        opt.srcWidth    = SRC_WIDTH;
        opt.srcHeight   = SRC_HEIGHT;
        opt.srcHStride  = src_vir_width;
        opt.srcVStride  = src_vir_height;
        opt.srcX   = 0;
        opt.srcY   = 0;
        if (wid_alig16 != SRC_WIDTH)
            opt.srcCrop8R = 1;
        if (hei_alig16 != SRC_HEIGHT)
            opt.srcCrop8D = 1;

        wid_alig16 = ((DST_WIDTH + 15) & (~0xf));
        hei_alig16 = ((DST_HEIGHT + 15) & (~0xf));

        opt.dstAddr     = dst.phy_addr;
        opt.dstFormat   = PP_OUT_FORMAT_YUV420INTERLAVE;
        opt.dstWidth    = DST_WIDTH;
        opt.dstHeight   = DST_HEIGHT;
        opt.dstHStride  = dst_vir_width;
        opt.dstVStride  = dst_vir_height;
        opt.dstX   = 0;
        opt.dstY   = 0;

        opt.deinterlace = 0;

        opt.rotation    = PP_ROTATION_RIGHT_90;//PP_ROTATION_RIGHT_90;//PP_ROTATION_RIGHT_90;//PP_ROTATION_RIGHT_90;

        opt.vpuFd       = vpuFd;
        ret |= android::ppOpInit(&hnd, &opt);
        if (ret) {
            ALOGE("ppOpInit failed");
            hnd = NULL;
            goto end;
        }
        ret = android::ppOpPerform(hnd);
        if (ret) {
            ALOGE("ppOpPerform failed");
        }
        ret = android::ppOpSync(hnd);
        if (ret) {
            ALOGE("ppOpSync failed");
        }
        ret = android::ppOpRelease(hnd);
        if (ret) {
            ALOGE("ppOpPerform failed");
        }

        VPUMemInvalidate(&dst);
        {
#if 0
            int i,  j, ret = 0;
            char *tmp = (char *)dst.vir_addr;
            /*for(i=0; i<3; i++)
            {
                printf("col out_pos=%d, 0x%x\n", i*DST_WIDTH, tmp[i*DST_WIDTH]);
            }
            for(i=0; i<3; i++)
            {
                printf("las out_pos=%d, 0x%x\n", (DST_HEIGHT-1-i)*DST_WIDTH, tmp[(DST_HEIGHT-1-i)*DST_WIDTH]);
            }*/
            for (i = 0; i < DST_HEIGHT; i++) {
                for (j = 0; j < DST_WIDTH; j++) {
                    if ( tmp[i * DST_WIDTH + j] != (i & 0xff))
                        ret = 1;
                }
            }
            if ( ret)
                printf("framecount=%d pp operation is failed!\n", (framecnt - 1));
            else
                printf("framecount=%d pp operation is suceess!\n", (framecnt - 1));

            memset(dst.vir_addr, 0xff, DST_SIZE);
            VPUMemClean(&dst);
#endif

#if 1
            char *tmp = (char *)dst.vir_addr;
            //memset(&tmp[DST_WIDTH*DST_HEIGHT], 0x80, DST_WIDTH*DST_HEIGHT/2);
            //VPUMemClean(&dst);
            fpw = fopen("/data/testout.yuv", "wb+");

            if (fpw)fwrite((char *)(dst.vir_addr), 1, dst_vir_width * dst_vir_height * 3 / 2, fpw);
            if (fpw)fclose(fpw);
#endif
        }
    }

end:
    if (tmpbuf)free(tmpbuf);
    if (src.phy_addr) VPUFreeLinear(&src);
    if (dst.phy_addr) VPUFreeLinear(&dst);
    if (vpuFd > 0) VPUClientRelease(vpuFd);
    ALOGI("ppOp test end\n");
    return 0;
}

#endif

