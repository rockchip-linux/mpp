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

#ifndef __MPP_DEC_CFG_IMPL_H__
#define __MPP_DEC_CFG_IMPL_H__

#include "mpp_trie.h"
#include "mpp_dec_cfg.h"

typedef struct MppDecCfgImpl_t {
    RK_S32              size;
    MppTrie             api;
    MppDecCfgSet        cfg;
} MppDecCfgImpl;

#endif /*__MPP_DEC_CFG_IMPL_H__*/
