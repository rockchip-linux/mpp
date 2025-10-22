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
#ifndef H265E_CODEC_H
#define H265E_CODEC_H

#include "mpp_debug.h"

#define H265E_DBG_FUNCTION          (0x00000001)
#define H265E_DBG_INPUT             (0x00000010)
#define H265E_DBG_OUTPUT            (0x00000020)
#define H265E_DBG_PS                (0x00000040)
#define H265E_DBG_DPB               (0x00000080)
#define H265E_DBG_DPB_REF           (0x000000c0)

#define H265E_DBG_SLICE             (0x00000100)
#define H265E_DBG_HEADER            (0x00000200)
#define H265E_DBG_API               (0x00000400)
#define H265E_DBG_SKIP              (0x00000800)

#define H265E_PS_BUF_SIZE           512
#define H265E_SEI_BUF_SIZE          1024
#define H265E_EXTRA_INFO_BUF_SIZE   (H265E_PS_BUF_SIZE + H265E_SEI_BUF_SIZE)

extern RK_U32 h265e_debug;

#define h265e_dbg(flag, fmt, ...)   _mpp_dbg(h265e_debug, flag, fmt, ## __VA_ARGS__)
#define h265e_dbg_f(flag, fmt, ...) _mpp_dbg_f(h265e_debug, flag, fmt, ## __VA_ARGS__)

#define h265e_dbg_func(fmt, ...)    h265e_dbg_f(H265E_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define h265e_dbg_input(fmt, ...)   h265e_dbg(H265E_DBG_INPUT, fmt, ## __VA_ARGS__)
#define h265e_dbg_output(fmt, ...)  h265e_dbg(H265E_DBG_OUTPUT, fmt, ## __VA_ARGS__)

#define h265e_dbg_ps(fmt, ...)      h265e_dbg(H265E_DBG_PS, fmt, ## __VA_ARGS__)
#define h265e_dbg_dpb(fmt, ...)     h265e_dbg(H265E_DBG_DPB, fmt, ## __VA_ARGS__)
#define h265e_dbg_slice(fmt, ...)   h265e_dbg(H265E_DBG_SLICE, fmt, ## __VA_ARGS__)
#define h265e_dbg_skip(fmt, ...)   h265e_dbg(H265E_DBG_SKIP, fmt, ## __VA_ARGS__)

#define h265e_dbg_dpb_ref(fmt, ...)  h265e_dbg(H265E_DBG_DPB_REF, fmt, ## __VA_ARGS__)

#endif
