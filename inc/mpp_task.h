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

#ifndef __MPP_TASK_H__
#define __MPP_TASK_H__

#include "mpp_frame.h"
#include "mpp_packet.h"

/*
 * Advanced task flow
 * Advanced task flow introduces three concepts: port, task and item
 *
 * Port is from OpenMAX
 * Port has two type: input port and output port which are all for data transaction.
 * Port work like a queue. task will be dequeue from or enqueue to one port.
 * On input side user will dequeue task from input port, setup task and enqueue task
 * back to input port.
 * On output side user will dequeue task from output port, get the information from
 * and then enqueue task back to output port.
 *
 * Task indicates one transaction on the port.
 * Task has two working mode: async mode and sync mode
 * If mpp is work in sync mode on task enqueue function return the task has been done
 * If mpp is work in async mode on task enqueue function return the task is just put
 * on the task queue for process.
 * Task can carry different items. Task just like a container of items
 *
 * Item indicates MppPacket or MppFrame which is contained in one task
 */

/*
 * mpp has two ports: input and output
 * Each port uses its task queue to communication
 */
typedef enum {
    MPP_PORT_INPUT,
    MPP_PORT_OUTPUT,
    MPP_PORT_BUTT,
} MppPortType;

/*
 * Advance task work flow mode:
 ******************************************************************************
 * 1. async mode (default)
 *
 * mpp_init(type, coding, MPP_WORK_ASYNC)
 *
 * input thread
 * a - dequeue(input, *task)
 * b - task_set_item(packet/frame)
 * c - enqueue(input, task)     // when enqueue return the task is not done yet
 *
 * output thread
 * a - dequeue(output, *task)
 * b - task_get_item(frame/packet)
 * c - enqueue(output, task)
 ******************************************************************************
 * 2. sync mode
 *
 * mpp_init(type, coding, MPP_WORK_SYNC)
 *
 * a - dequeue(input, *task)
 * b - task_set_item(packet/frame)
 * c - enqueue(task)            // when enqueue return the task is finished
 ******************************************************************************
 */
typedef enum {
    MPP_TASK_ASYNC,
    MPP_TASK_SYNC,
    MPP_TASK_WORK_MODE_BUTT,
} MppTaskWorkMode;

/*
 * MppTask is descriptor of a task which send to mpp for process
 * mpp can support different type of work mode, for example:
 *
 * decoder:
 *
 * 1. typical decoder mode:
 * input    - MppPacket     (normal cpu buffer, need cpu copy)
 * output   - MppFrame      (ion/drm buffer in external/internal mode)
 * 2. secure decoder mode:
 * input    - MppPacket     (externel ion/drm buffer, cpu can not access)
 * output   - MppFrame      (ion/drm buffer in external/internal mode, cpu can not access)
 *
 * interface usage:
 *
 * typical flow
 * input side:
 * task_dequeue(ctx, PORT_INPUT, &task);
 * task_put_item(task, MODE_INPUT, packet)
 * task_enqueue(ctx, PORT_INPUT, task);
 * output side:
 * task_dequeue(ctx, PORT_OUTPUT, &task);
 * task_get_item(task, MODE_OUTPUT, &frame)
 * task_enqueue(ctx, PORT_OUTPUT, task);
 *
 * secure flow
 * input side:
 * task_dequeue(ctx, PORT_INPUT, &task);
 * task_put_item(task, MODE_INPUT, packet)
 * task_put_item(task, MODE_OUTPUT, frame)  // buffer will be specified here
 * task_enqueue(ctx, PORT_INPUT, task);
 * output side:
 * task_dequeue(ctx, PORT_OUTPUT, &task);
 * task_get_item(task, MODE_OUTPUT, &frame)
 * task_enqueue(ctx, PORT_OUTPUT, task);
 *
 * encoder:
 *
 * 1. typical encoder mode:
 * input    - MppFrame      (ion/drm buffer in external mode)
 * output   - MppPacket     (normal cpu buffer, need cpu copy)
 * 2. user input encoder mode:
 * input    - MppFrame      (normal cpu buffer, need to build hardware table for this buffer)
 * output   - MppPacket     (normal cpu buffer, need cpu copy)
 * 3. secure encoder mode:
 * input    - MppFrame      (ion/drm buffer in external mode, cpu can not access)
 * output   - MppPacket     (externel ion/drm buffer, cpu can not access)
 *
 * typical / user input flow
 * input side:
 * task_dequeue(ctx, PORT_INPUT, &task);
 * task_put_item(task, MODE_INPUT, frame)
 * task_enqueue(ctx, PORT_INPUT, task);
 * output side:
 * task_dequeue(ctx, PORT_OUTPUT, &task);
 * task_get_item(task, MODE_OUTPUT, &packet)
 * task_enqueue(ctx, PORT_OUTPUT, task);
 *
 * secure flow
 * input side:
 * task_dequeue(ctx, PORT_INPUT, &task);
 * task_put_item(task, MODE_OUTPUT, packet)  // buffer will be specified here
 * task_put_item(task, MODE_INPUT, frame)
 * task_enqueue(ctx, PORT_INPUT, task);
 * output side:
 * task_dequeue(ctx, PORT_OUTPUT, &task);
 * task_get_item(task, MODE_OUTPUT, &packet)
 * task_enqueue(ctx, PORT_OUTPUT, task);
 *
 * image processing
 *
 * 1. typical image process mode:
 * input    - MppFrame      (ion/drm buffer in external mode)
 * output   - MppFrame      (ion/drm buffer in external mode)
 *
 * typical / user input flow
 * input side:
 * task_dequeue(ctx, PORT_INPUT, &task);
 * task_put_item(task, MODE_INPUT, frame)
 * task_enqueue(ctx, PORT_INPUT, task);
 * output side:
 * task_dequeue(ctx, PORT_OUTPUT, &task);
 * task_get_item(task, MODE_OUTPUT, &frame)
 * task_enqueue(ctx, PORT_OUTPUT, task);
 */
/* NOTE: use index rather then handle to descripbe task */
typedef RK_S32  MppTask;
typedef void*   MppItem;

typedef enum {
    ITEM_MODE_INPUT,
    ITEM_MODE_OUTPUT,
    ITEM_MODE_BUTT,
} MppItemMode;

typedef enum {
    ITEM_TYPE_PACKET,
    ITEM_TYPE_FRAME,
    ITEM_TYPE_CONFIG,
    ITEM_TYPE_BUTT,
} MppItemType;

#ifdef __cplusplus
extern "C" {
#endif

void mpp_task_set_item(MppTask task, MppItemMode mode, MppItemType type, MppItem item);
void mpp_task_get_item(MppTask task, MppItemMode mode, MppItemType type, MppItem *item);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_QUEUE_H__*/
