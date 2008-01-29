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

#ifndef __MPP_SYNTAX_H__
#define __MPP_SYNTAX_H__

#include "mpp_err.h"
#include "mpp_list.h"

#define MPP_SYNTAX_MAX_NUMBER       8

/*
 * modified by parser
 *
 * number   : the number of the data pointer array element
 * data     : the address of the pointer array, parser will add its data here
 */
typedef struct {
    RK_U32              number;
    void                *data;
} MppSyntax;

typedef void* MppSyntaxHnd;
typedef void* MppSyntaxGroup;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * group init / deinit will be called by hal
 */
MPP_RET mpp_syntax_group_init(MppSyntaxGroup *group, RK_U32 count);
MPP_RET mpp_syntax_group_deinit(MppSyntaxGroup group);

/*
 * normal working flow:
 *
 * dec:
 *
 * get_hnd(group, 0, &hnd)      - dec get a unused handle first
 * parser->parse                - parser write a local info in dec
 * set_info(hnd, info)          - dec write the local info to handle
 * set_used(hnd, 1)             - decoder set handle to used
 *
 * hal:
 * get_hnd(group, 1, &hnd)
 * read_info(hnd, info)
 * set_used(hnd, 0)
 *
 * these calls do not own syntax handle but just get its reference
 * so there is not need to free or destory the handle
 *
 */
MPP_RET mpp_syntax_get_hnd(MppSyntaxGroup group, RK_U32 used, MppSyntaxHnd *hnd);
MPP_RET mpp_syntax_set_used(MppSyntaxHnd hnd, RK_U32 used);

MPP_RET mpp_syntax_get_info(MppSyntaxHnd hnd, MppSyntax *syntax);
MPP_RET mpp_syntax_set_info(MppSyntaxHnd hnd, MppSyntax *syntax);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_SYNTAX_H__*/

