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

#ifndef AV1_ENTROPYMODE_H_
#define AV1_ENTROPYMODE_H_

#include "av1d_common.h"

#define DEFAULT_COMP_INTRA_PROB 32

#define AV1_DEF_INTERINTRA_PROB 248
#define AV1_UPD_INTERINTRA_PROB 192
#define SEPARATE_INTERINTRA_UV 0

typedef RK_U8 av1_prob;

extern const RK_S8 av1hwd_intra_mode_tree[];

extern const av1_prob av1_kf_default_bmode_probs[AV1_INTRA_MODES]
[AV1_INTRA_MODES]
[AV1_INTRA_MODES - 1];

extern const RK_S8 av1hwd_intra_mode_tree[];

extern const RK_S8 av1_sb_mv_ref_tree[];

/* probability models for partition information */
extern const RK_S8 av1hwd_partition_tree[];
// extern struct av1_token av1_partition_encodings[PARTITION_TYPES];
// extern const av1_prob av1_partition_probs[NUM_FRAME_TYPES]
//                                          [NUM_PARTITION_CONTEXTS]
//                                          [PARTITION_TYPES];

void Av1EntropyModeInit(void);
void AV1SetDefaultCDFs(AV1CDFs *cdfs, MvCDFs *cdfs_ndvc);
void Av1DefaultCoeffProbs(RK_U32 base_qindex, void *ptr);
struct AV1Common;

// void Av1InitMbmodeProbs(struct Av1Decoder *x);

// extern void Av1InitModeContexts(struct Av1Decoder *pc);

extern const enum InterpolationFilterType av1hwd_switchable_interp[AV1_SWITCHABLE_FILTERS];

extern const int av1hwd_switchable_interp_map[SWITCHABLE + 1];

extern const RK_S8 av1hwd_switchable_interp_tree[2 * (AV1_SWITCHABLE_FILTERS - 1)];

// extern struct av1_token av1hwd_switchable_interp_encodings[AV1_SWITCHABLE_FILTERS];

extern const av1_prob av1hwd_switchable_interp_prob[AV1_SWITCHABLE_FILTERS + 1][AV1_SWITCHABLE_FILTERS - 1];

extern const av1_prob av1_default_tx_probs_32x32p[TX_SIZE_CONTEXTS] [TX_SIZE_MAX_SB - 1];
extern const av1_prob av1_default_tx_probs_16x16p[TX_SIZE_CONTEXTS] [TX_SIZE_MAX_SB - 2];
extern const av1_prob av1_default_tx_probs_8x8p[TX_SIZE_CONTEXTS] [TX_SIZE_MAX_SB - 3];

extern const av1_prob av1_default_intra_ext_tx_prob[EXT_TX_SIZES][TX_TYPES] [TX_TYPES - 1];
extern const av1_prob av1_default_inter_ext_tx_prob[EXT_TX_SIZES][TX_TYPES - 1];

extern const RK_S8 av1_segment_tree[];

#endif
