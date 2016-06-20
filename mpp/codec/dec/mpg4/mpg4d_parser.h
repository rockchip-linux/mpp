/*
 *
 * Copyright 2010 Rockchip Electronics Co. LTD
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

#ifndef __MPG4D_PARSER_H__
#define __MPG4D_PARSER_H__

#include "mpp_log.h"
#include "mpp_packet.h"
#include "mpp_buf_slot.h"
#include "hal_task.h"

extern RK_U32 mpg4d_debug;
#define MPG4D_DBG_FUNCTION          (0x00000001)
#define MPG4D_DBG_PPS               (0x00000008)
#define MPG4D_DBG_SLICE_HDR         (0x00000010)
#define MPG4D_DBG_REF               (0x00000080)
#define MPG4D_DBG_TIME              (0x00000100)

#define mpg4d_dbg(flag, fmt, ...)   _mpp_dbg(mpg4d_debug, flag, fmt, ## __VA_ARGS__)

typedef void* Mpg4dParser;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_mpg4_parser_init(Mpg4dParser *ctx, MppBufSlots frame_slots);
MPP_RET mpp_mpg4_parser_deinit(Mpg4dParser ctx);
MPP_RET mpp_mpg4_parser_flush(Mpg4dParser ctx);
MPP_RET mpp_mpg4_parser_reset(Mpg4dParser ctx);

MPP_RET mpp_mpg4_parser_split(Mpg4dParser ctx, MppPacket dst, MppPacket src);
MPP_RET mpp_mpg4_parser_decode(Mpg4dParser ctx, MppPacket pkt);
MPP_RET mpp_mpg4_parser_setup_syntax(Mpg4dParser ctx, MppSyntax syntax);
MPP_RET mpp_mpg4_parser_setup_output(Mpg4dParser ctx, RK_S32 *output);
MPP_RET mpp_mpg4_parser_setup_refer(Mpg4dParser ctx, RK_S32 *refer, RK_S32 max_ref);
MPP_RET mpp_mpg4_parser_setup_display(Mpg4dParser ctx);

#ifdef __cplusplus
}
#endif

#endif/* __MPG4D_PAESER_H__ */
