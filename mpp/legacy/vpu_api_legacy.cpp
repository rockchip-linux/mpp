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

#define MODULE_TAG "vpu_api_legacy"

#include "mpp_log.h"

#include "vpu_api_legacy.h"

VpuApi::VpuApi()
{
    mpp_log_f("in\n");
    mpp_log_f("ok\n");
}

VpuApi::~VpuApi()
{
    mpp_log_f("in\n");
    mpp_log_f("ok\n");
}

RK_S32 VpuApi::init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)extraData;
    (void)extra_size;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::send_stream(RK_U8* buf, RK_U32 size, RK_S64 timestamp, RK_S32 usePts)
{
    mpp_log_f("in\n");
    (void)buf;
    (void)size;
    (void)timestamp;
    (void)usePts;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::get_frame(DecoderOut_t *aDecOut)
{
    mpp_log_f("in\n");
    (void)aDecOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::flush(VpuCodecContext *ctx)
{
    mpp_log_f("in\n");
    (void)ctx;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)pkt;
    (void)aDecOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::decode_sendstream(VpuCodecContext *ctx, VideoPacket_t *pkt)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)pkt;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi:: decode_getoutframe(VpuCodecContext *ctx, DecoderOut_t *aDecOut)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)aDecOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)aEncInStrm;
    (void)aEncOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::encoder_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)aEncInStrm;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::encoder_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)aEncOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::perform(RK_U32 cmd, RK_U32 *data)
{
    mpp_log_f("in\n");
    (void)cmd;
    (void)data;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::control(VpuCodecContext *ctx, VPU_API_CMD cmd, void *param)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)cmd;
    (void)param;
    mpp_log_f("ok\n");
    return 0;
}

