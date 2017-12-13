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

#ifndef __MPP_RUNTIME__
#define __MPP_RUNTIME__

#include "rk_mpi.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Runtime function detection is to support different binary on different
 * runtime environment. This is usefull on product environemnt.
 */
RK_U32 mpp_rt_allcator_is_valid(MppBufferType type);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_RUNTIME__*/

