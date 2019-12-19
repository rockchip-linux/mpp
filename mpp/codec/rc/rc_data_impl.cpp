/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "rc_data_impl"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_common.h"

#include "rc_data.h"
#include "rc_data_impl.h"

#define RC_LIST_HEAD            1
#define RC_LIST_TAIL            0

#define RC_FRM_TYPE_I           0
#define RC_FRM_TYPE_P           1

#define RC_FRM_STATUS_VALID     (0x00000001)
#define RC_HAL_SET_VALID        (0x00000002)
#define RC_HAL_RET_VALID        (0x00000004)

static void rc_data_indexes_init(RcDataIndexes *sort)
{
    RK_S32 i;

    memset(sort, 0, sizeof(*sort));

    INIT_LIST_HEAD(&sort->seq);

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(sort->type); i++)
        INIT_LIST_HEAD(&sort->type[i]);

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(sort->tid); i++)
        INIT_LIST_HEAD(&sort->tid[i]);

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(sort->status); i++)
        INIT_LIST_HEAD(&sort->status[i]);
}

static void rc_data_node_init(DataGroupImpl *p, RcDataNode *node, RK_S32 slot_idx)
{
    RcDataIndexes *indexes = &p->indexes;
    RcDataStatus status = RC_DATA_ST_UNUSED;
    RcDataHead *head = &node->head;
    RcDataBase *base = &node->base;

    memset(node, 0, sizeof(*node));

    head->node = node;
    head->slot_id = slot_idx;

    INIT_LIST_HEAD(&head->seq);
    INIT_LIST_HEAD(&head->type);
    INIT_LIST_HEAD(&head->tid);
    INIT_LIST_HEAD(&head->status);
    list_add_tail(&head->status, &indexes->status[status]);
    indexes->status_cnt[status]++;

    base->head = head;

    node->extra = NULL;
}

MPP_RET rc_data_group_init(DataGroupImpl *p, RK_S32 base_cnt, RK_S32 extra_cnt)
{
    p->lock = new Mutex();

    node_group_init(&p->node, sizeof(RcDataNode), base_cnt);
    node_group_init(&p->extra, sizeof(RcDataExtra), extra_cnt);

    mpp_assert(p->lock);
    mpp_assert(p->node);
    mpp_assert(p->extra);

    p->base_cnt = base_cnt;
    p->extra_cnt = extra_cnt;

    rc_data_group_reset(p);

    return MPP_OK;
}

MPP_RET rc_data_group_deinit(DataGroupImpl *p)
{
    mpp_assert(p->lock);
    mpp_assert(p->node);
    mpp_assert(p->extra);

    node_group_deinit(p->node);
    node_group_deinit(p->extra);

    delete p->lock;

    return MPP_OK;
}

MPP_RET rc_data_group_reset(DataGroupImpl *p)
{
    RK_S32 i;

    rc_data_indexes_init(&p->indexes);

    for (i = 0; i < p->base_cnt; i++) {
        RcDataNode *node = (RcDataNode *)node_group_get(p->node, i);

        rc_data_node_init(p, node, i);
    }

    for (i = 0; i < p->extra_cnt; i++) {
        RcDataExtra *extra = (RcDataExtra *)node_group_get(p->extra, i);

        /* NOTE: node in head is NULL */
        memset(extra, 0, sizeof(*extra));
    }

    return MPP_OK;
}

RcDataNode *rc_data_group_get_node_by_seq_id(DataGroupImpl *p, RK_S32 seq_id)
{
    RcDataIndexes *indexes = &p->indexes;

    if (list_empty(&indexes->seq))
        return NULL;

    RcDataNode *node = NULL;
    RcDataHead *pos;

    if (MPP_ABS(seq_id - indexes->seq_new) < MPP_ABS(seq_id - indexes->seq_old)) {
        list_for_each_entry(pos, &indexes->seq, RcDataHead, seq) {
            if (pos->seq_id == seq_id) {
                node = pos->node;
                break;
            }
        }
    } else {
        list_for_each_entry_reverse(pos, &indexes->seq, RcDataHead, seq) {
            if (pos->seq_id == seq_id) {
                node = pos->node;
                break;
            }
        }
    }

    return node;
}

RcDataNode *rc_data_group_get_node_by_status(DataGroupImpl *p, RcDataStatus status, RK_S32 whence)
{
    RcDataIndexes *indexes = &p->indexes;

    mpp_assert(status >= RC_DATA_ST_UNUSED && status <= RC_DATA_ST_DONE);

    struct list_head *list = &indexes->status[status];

    if (list_empty(list))
        return NULL;

    list = (whence == RC_LIST_HEAD) ? (list->next) : (list->prev);
    RcDataHead *head = list_entry(list, RcDataHead, status);
    mpp_assert(head->data_status == status);
    mpp_assert(head->node);
    return head->node;
}

void rc_data_group_put_node(DataGroupImpl *p, RcDataNode *node)
{
    RcDataIndexes *indexes = &p->indexes;
    RcDataHead *head = &node->head;

    AutoMutex auto_lock(p->lock);
    RcDataStatus data_status = head->data_status;

    list_del_init(&head->status);
    indexes->status_cnt[data_status]--;

    if (data_status == RC_DATA_ST_FILLING) {
        // ready on filling add to indexing list by property
        EncFrmStatus frm_status = head->frm_status;
        RK_S32 type_id = frm_status.is_intra ? RC_FRM_TYPE_I : RC_FRM_TYPE_P;
        RK_S32 tid = frm_status.temporal_id;

        mpp_assert(list_empty(&head->seq));
        list_add_tail(&head->seq, &indexes->seq);
        indexes->seq_cnt++;
        indexes->seq_new = head->seq_id;

        mpp_assert(list_empty(&head->type));
        list_add_tail(&head->type, &indexes->type[type_id]);
        indexes->type_cnt[type_id]++;

        mpp_assert(tid < 4);
        mpp_assert(list_empty(&head->tid));
        list_add_tail(&head->tid, &indexes->tid[type_id]);
        indexes->tid_cnt[type_id]++;
    }

    // goto next status
    switch (data_status) {
    case RC_DATA_ST_UNUSED : {
        data_status = RC_DATA_ST_FILLING;
    } break;
    case RC_DATA_ST_FILLING : {
        data_status = RC_DATA_ST_DONE;
    } break;
    case RC_DATA_ST_DONE : {
        data_status = RC_DATA_ST_UNUSED;
    } break;
    default : {
        data_status = RC_DATA_ST_BUTT;
        mpp_assert(data_status != RC_DATA_ST_BUTT);
    } break;
    }

    if (data_status == RC_DATA_ST_UNUSED) {
        // removed from indexing list
        EncFrmStatus frm_status = head->frm_status;

        if (!list_empty(&head->seq)) {
            list_del_init(&head->seq);
            indexes->seq_cnt--;
            indexes->seq_old = head->seq_id;
        }

        if (!list_empty(&head->type)) {
            RK_S32 type_id = frm_status.is_intra ? RC_FRM_TYPE_I : RC_FRM_TYPE_P;

            list_del_init(&head->type);
            indexes->type_cnt[type_id]--;
        }

        if (!list_empty(&head->tid)) {
            RK_S32 tid = frm_status.temporal_id;

            mpp_assert(tid < 4);
            list_del_init(&head->tid);
            indexes->tid_cnt[tid]--;
        }
    }

    list_add_tail(&head->status, &indexes->status[data_status]);
    indexes->status_cnt[data_status]++;
    head->data_status = data_status;
}

RcData rc_data_get_next(DataGroup grp)
{
    DataGroupImpl *p = (DataGroupImpl *)grp;
    RcDataNode *node = rc_data_group_get_node_by_status(
                           p, RC_DATA_ST_UNUSED, RC_LIST_HEAD);

    return (RcData)node;
}

RcData rc_data_get_curr_latest(DataGroup grp)
{
    DataGroupImpl *p = (DataGroupImpl *)grp;
    RcDataNode *node = rc_data_group_get_node_by_status(
                           p, RC_DATA_ST_FILLING, RC_LIST_TAIL);

    return (RcData)node;
}

RcData rc_data_get_curr_oldest(DataGroup grp)
{
    DataGroupImpl *p = (DataGroupImpl *)grp;
    RcDataNode *node = rc_data_group_get_node_by_status(
                           p, RC_DATA_ST_FILLING, RC_LIST_HEAD);

    return (RcData)node;
}

RcData rc_data_get_last_latest(DataGroup grp)
{
    DataGroupImpl *p = (DataGroupImpl *)grp;
    RcDataNode *node = rc_data_group_get_node_by_status(
                           p, RC_DATA_ST_DONE, RC_LIST_TAIL);

    return (RcData)node;
}

RcData rc_data_get_last_oldest(DataGroup grp)
{
    DataGroupImpl *p = (DataGroupImpl *)grp;
    RcDataNode *node = rc_data_group_get_node_by_status(
                           p, RC_DATA_ST_DONE, RC_LIST_HEAD);

    return (RcData)node;
}
