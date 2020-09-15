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

#ifndef __H265E_CONTEXT_TABLE_H__
#define __H265E_CONTEXT_TABLE_H__

#include "rk_type.h"

#define NUM_SPLIT_FLAG_CTX          3       ///< number of context models for split flag
#define NUM_SKIP_FLAG_CTX           3       ///< number of context models for skip flag
#define NUM_MERGE_FLAG_EXT_CTX      1       ///< number of context models for merge flag of merge extended
#define NUM_MERGE_IDX_EXT_CTX       1       ///< number of context models for merge index of merge extended

#define OFF_SPLIT_FLAG_CTX                  (0)
#define OFF_SKIP_FLAG_CTX                   (OFF_SPLIT_FLAG_CTX         +     NUM_SPLIT_FLAG_CTX)//3
#define OFF_MERGE_FLAG_EXT_CTX              (OFF_SKIP_FLAG_CTX          +     NUM_SKIP_FLAG_CTX)//3
#define OFF_MERGE_IDX_EXT_CTX               (OFF_MERGE_FLAG_EXT_CTX     +     NUM_MERGE_FLAG_EXT_CTX)//1
#define MAX_OFF_CTX_MOD                     (OFF_MERGE_IDX_EXT_CTX      +     NUM_MERGE_IDX_EXT_CTX)
#define CNU                         154     ///< dummy initialization value for unused context models 'Context model Not Used'

extern const uint8_t g_nextState[128][2];

#define sbacGetMps(S)               ((S) & 1)
#define sbacGetState(S)             ((S) >> 1)
#define sbacNext(S, V)              (g_nextState[(S)][(V)])

static const RK_U8 INIT_SPLIT_FLAG[3][NUM_SPLIT_FLAG_CTX] = {
    { 107,  139,  126, },
    { 107,  139,  126, },
    { 139,  141,  157, },
};

static const RK_U8 INIT_SKIP_FLAG[3][NUM_SKIP_FLAG_CTX] = {
    { 197,  185,  201, },
    { 197,  185,  201, },
    { CNU,  CNU,  CNU, },
};

static const RK_U8 INIT_MERGE_FLAG_EXT[3][NUM_MERGE_FLAG_EXT_CTX] = {
    { 154, },
    { 110, },
    { CNU, },
};

static const RK_U8 INIT_MERGE_IDX_EXT[3][NUM_MERGE_IDX_EXT_CTX] = {
    { 137, },
    { 122, },
    { CNU, },
};

#endif /* __H265E_CONTEXT_TABLE_H__ */