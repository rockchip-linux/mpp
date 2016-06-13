/*
 *
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

/*
 * @file       h265d_defs.h
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */


#ifndef __H265D_DEF_H__
#define __H265D_DEF_H__

#ifdef VPS_EXTENSION
#define MAX_VPS_NUM_SCALABILITY_TYPES       16
#define MAX_VPS_LAYER_ID_PLUS1              MAX_LAYERS
#define MAX_VPS_LAYER_SETS_PLUS1            1024
#define VPS_EXTN_MASK_AND_DIM_INFO          1
#define VPS_MOVE_DIR_DEPENDENCY_FLAG        1
#define VPS_EXTN_DIRECT_REF_LAYERS          1
#define VPS_EXTN_PROFILE_INFO               1
#define VPS_PROFILE_OUTPUT_LAYERS           1
#define VPS_EXTN_OP_LAYER_SETS              1
#endif
#define DERIVE_LAYER_ID_LIST_VARIABLES      1

#endif /* H265D_DEF_H */
