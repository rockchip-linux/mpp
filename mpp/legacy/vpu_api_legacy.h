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

#ifndef __VPU_API_LEGACY_H__
#define __VPU_API_LEGACY_H__

#include <stdio.h>

#include "vpu_api.h"
#include "rk_mpi.h"
#include "rk_venc_cfg.h"

#include "vpu_api_mlvec.h"

#define OMX_BUFFERFLAG_EOS              0x00000001

#define VPU_API_DBG_FUNCTION            (0x00000001)
#define VPU_API_DBG_INPUT               (0x00000010)
#define VPU_API_DBG_OUTPUT              (0x00000020)
#define VPU_API_DBG_CONTROL             (0x00000040)

#define vpu_api_dbg(flag, fmt, ...)     _mpp_dbg(vpu_api_debug, flag, fmt, ## __VA_ARGS__)
#define vpu_api_dbg_f(flag, fmt, ...)   _mpp_dbg_f(vpu_api_debug, flag, fmt, ## __VA_ARGS__)

#define vpu_api_dbg_func(fmt, ...)      vpu_api_dbg_f(VPU_API_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define vpu_api_dbg_input(fmt, ...)     vpu_api_dbg_f(VPU_API_DBG_INPUT, fmt, ## __VA_ARGS__)
#define vpu_api_dbg_output(fmt, ...)    vpu_api_dbg_f(VPU_API_DBG_OUTPUT, fmt, ## __VA_ARGS__)
#define vpu_api_dbg_ctrl(fmt, ...)      vpu_api_dbg_f(VPU_API_DBG_CONTROL, fmt, ## __VA_ARGS__)

extern RK_U32 vpu_api_debug;

typedef enum {
    INPUT_FORMAT_MAP,
} PerformCmd;

class VpuApiLegacy
{
public:
    VpuApiLegacy();
    ~VpuApiLegacy();

    RK_S32 init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size);
    RK_S32 flush(VpuCodecContext *ctx);

    RK_S32 decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut);
    RK_S32 decode_sendstream(VideoPacket_t *pkt);
    RK_S32 decode_getoutframe(DecoderOut_t *aDecOut);
    RK_S32 preProcessPacket(VpuCodecContext *ctx, VideoPacket_t *pkt);

    RK_S32 encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut);
    RK_S32 encoder_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm);
    RK_S32 encoder_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut);

    RK_S32 perform(PerformCmd cmd, RK_S32 *data);
    RK_S32 control(VpuCodecContext *ctx, VPU_API_CMD cmd, void *param);

private:
    VPU_GENERIC vpug;
    MppCtx mpp_ctx;
    MppApi *mpi;
    RK_U32 init_ok;
    RK_U32 frame_count;
    RK_U32 set_eos;

    /* encoder parameters */
    MppBufferGroup memGroup;
    MppFrameFormat format;

    RK_S32 fd_input;
    RK_S32 fd_output;

    RK_U32 mEosSet;

    EncParameter_t enc_param;
    MppEncCfg enc_cfg;
    MppPacket enc_hdr_pkt;
    void *enc_hdr_buf;
    RK_S32 enc_hdr_buf_size;

    /* for mlvec */
    VpuApiMlvec mlvec;
    VpuApiMlvecDynamicCfg mlvec_dy_cfg;
};

#endif /*__VPU_API_LEGACY_H__*/
