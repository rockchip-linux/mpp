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

#ifndef __H264E_SPS_H__
#define __H264E_SPS_H__

#include "mpp_platform.h"

#include "mpp_packet.h"
#include "mpp_enc_cfg.h"

#include "h264e_syntax_new.h"

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h264e_sps_update(SynH264eSps *sps, MppEncCfgSet *cfg, MppDeviceId dev);
MPP_RET h264e_sps_to_packet(SynH264eSps *sps, MppPacket packet);
MPP_RET h264e_sps_dump(SynH264eSps *sps);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_SPS_H__ */
