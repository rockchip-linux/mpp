/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#ifndef __VEPU580_TUNE_H__
#define __VEPU580_TUNE_H__

#define FRAME_CONTENT_ANALYSIS_NUM  5
#define FRAME_MOTION_ANALYSIS_NUM   5
#define MD_WIN_LEN                  5
#define MD_SHOW_LEN                 4

typedef enum MotionLevel_e {
    MOTION_LEVEL_STILL              = 0,
    MOTION_LEVEL_MOTION             = 1,
    MOTION_LEVEL_BIG_MOTION         = 2,
    MOTION_LEVEL_UNKNOW_SCENE       = 3,
} MotionLevel;

extern RK_S32 ctu_madp_cnt_thd[6][8];
extern RK_S32 madp_num_map[5][4];

#endif
