/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_QUEUE_H
#define MPP_QUEUE_H

#include "mpp_list.h"

typedef struct MppQueue_t {
    MppList* list;
    sem_t queue_pending;
    int flush_flag;
} MppQueue;

#endif
