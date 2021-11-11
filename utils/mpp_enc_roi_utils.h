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

#include "rk_venc_cmd.h"

typedef void* MppEncRoiCtx;

/*
 * NOTE: this structure is changeful. Do NOT expect binary compatible on it.
 */
typedef struct RRegion_t {
    RK_U16              x;              /**< horizontal position of top left corner */
    RK_U16              y;              /**< vertical position of top left corner */
    RK_U16              w;              /**< width of ROI rectangle */
    RK_U16              h;              /**< height of ROI rectangle */

    RK_S32              force_intra;    /**< flag of forced intra macroblock */
    RK_S32              qp_mode;        /**< 0 - relative qp 1 - absolute qp */
    RK_S32              qp_val;         /**< absolute / relative qp of macroblock */
} RoiRegionCfg;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_enc_roi_init(MppEncRoiCtx *ctx, RK_U32 w, RK_U32 h, MppCodingType type, RK_S32 count);
MPP_RET mpp_enc_roi_deinit(MppEncRoiCtx ctx);

MPP_RET mpp_enc_roi_add_region(MppEncRoiCtx ctx, RoiRegionCfg *region);
MPP_RET mpp_enc_roi_setup_meta(MppEncRoiCtx ctx, MppMeta meta);

#ifdef __cplusplus
}
#endif
