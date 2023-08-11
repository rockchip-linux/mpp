/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vepu580_tune"

#include "rk_type.h"
#include "vepu580_tune.h"

RK_S32 ctu_madp_cnt_thd[6][8] = {
    {50,  100, 130, 50,  100, 550, 500, 550},
    {100, 150, 200, 80,  120, 500, 450, 550},
    {150, 200, 250, 100, 150, 450, 400, 450},
    {50,  100, 130, 50,  100, 550, 500, 550},
    {100, 150, 200, 80,  120, 500, 450, 550},
    {150, 200, 250, 100, 150, 450, 400, 450}
};

RK_S32 madp_num_map[5][4] = {
    {0, 0, 0, 1},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 0},
    {1, 1, 1, 1},
};
