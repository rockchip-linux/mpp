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

#ifndef __MPP_OPT_H__
#define __MPP_OPT_H__

#include "rk_type.h"

/*
 * return zero or positive for option parser aproach step
 * return negative for failure or option help
 */
typedef RK_S32 (*OptParser)(void *ctx, const char *next);

typedef struct MppOptInfo_t {
    const char*     name;
    const char*     full_name;
    const char*     help;
    OptParser       proc;
} MppOptInfo;

typedef void* MppOpt;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_opt_init(MppOpt *opt);
MPP_RET mpp_opt_deinit(MppOpt opt);

MPP_RET mpp_opt_setup(MppOpt opt, void *ctx, RK_S32 node_cnt, RK_S32 opt_cnt);
/* Add NULL info to mark end of options */
MPP_RET mpp_opt_add(MppOpt opt, MppOptInfo *info);

MPP_RET mpp_opt_parse(MppOpt opt, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_OPT_H__*/
