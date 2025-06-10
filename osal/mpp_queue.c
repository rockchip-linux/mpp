/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd.
 */

#include "mpp_mem.h"
#include "mpp_queue.h"

MppQueue* mpp_queue_create(node_destructor func)
{
    MppQueue *queue = mpp_malloc(MppQueue, 1);

    queue->list = mpp_list_create(func);
    sem_init(&queue->queue_pending, 0, 0);
    queue->flush_flag = 0;

    return queue;
}

void mpp_queue_destroy(MppQueue *queue)
{
    mpp_list_destroy(queue->list);
    mpp_free(queue);
}

rk_s32 mpp_queue_push(MppQueue *queue, void *data, rk_s32 size)
{
    rk_s32 ret = rk_ok;

    ret = mpp_list_add_at_tail(queue->list, data, size);
    queue->flush_flag = 0;
    sem_post(&queue->queue_pending);

    return ret;
}

rk_s32 mpp_queue_pull(MppQueue *queue, void *data, rk_s32 size)
{
    rk_s32 ret = rk_ok;

    if (!queue->flush_flag)
        sem_wait(&queue->queue_pending);

    mpp_mutex_cond_lock(&queue->list->cond_lock);
    if (!mpp_list_size(queue->list)) {
        mpp_mutex_cond_unlock(&queue->list->cond_lock);
        return ret;
    }

    ret = mpp_list_del_at_head(queue->list, data, size);
    mpp_mutex_cond_unlock(&queue->list->cond_lock);

    return ret;
}

rk_s32 mpp_queue_flush(MppQueue *queue)
{
    if (queue->flush_flag)
        return 0;

    queue->flush_flag = 1;
    sem_post(&queue->queue_pending);
    mpp_list_flush(queue->list);

    return 0;
}
