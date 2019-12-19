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

#ifndef __RC_DATA_IMPL_H__
#define __RC_DATA_IMPL_H__

#include "rc_data_base.h"

typedef enum RcDataStatus_e {
    RC_DATA_ST_UNUSED,
    RC_DATA_ST_FILLING,
    RC_DATA_ST_DONE,
    RC_DATA_ST_BUTT,
} RcDataStatus;

/*
 * RcDataNode is composed of several parts:
 *
 * Sort function node   - for RcDataNode sorting
 * Basic data node      - basic long-term data storage
 * Extra data node      - extra short-term data storage
 */
typedef struct RcDataNode_t RcDataNode;

typedef struct RcDataHead_t {
    // node pointer to RcDataNode
    RcDataNode          *node;

    /* unique sequential id */
    RK_S32              seq_id;
    /* frame status parameter */
    EncFrmStatus        frm_status;
    /* slot index in storage */
    RK_S32              slot_id;

    /* list head to sequence list */
    struct list_head    seq;
    /* list head to frame type list */
    struct list_head    type;
    /* list head to temporal id list */
    struct list_head    tid;
    /* list head to status list */
    struct list_head    status;

    RcDataStatus        data_status;
} RcDataHead;

typedef struct RcDataBase_t {
    // node pointer to RcDataNode
    RcDataHead          *head;

    RK_S32              stream_size;
    RK_S32              qp_sum;
} RcDataBase;

/*
 * RcDataExtra is the short-term detail data storage.
 *
 * RC module will keep this frame data history for RcImplApi to implement
 * different rate control strategy.
 */
typedef struct RcDataExtra_t {
    // node pointer to RcDataNode
    RcDataHead          *head;

    RK_S32              target_bit;

    RcHalSet            set;
    RcHalRet            ret;
} RcDataExtra;

typedef struct RcDataIndexes_t {
    /* list head to sequence list */
    struct list_head    seq;
    RK_S32              seq_cnt;
    RK_S32              seq_new;
    RK_S32              seq_old;

    /*
     * list head to frame type list
     * 0 - I frame
     * 1 - P frame
     */
    struct list_head    type[2];
    RK_S32              type_cnt[2];

    /*
     * list head to temporal layer list
     * 0 - layer 0
     * 1 - layer 1
     * 2 - layer 2
     * 3 - layer 3
     */
    struct list_head    tid[4];
    RK_S32              tid_cnt[4];

    /*
     * list head to status list
     * 0 - unused
     * 1 - filling
     * 2 - fill done
     */
    struct list_head    status[3];
    RK_S32              status_cnt[4];
} RcDataIndexes;

typedef struct RcDataNode_t {
    RcDataHead          head;
    RcDataBase          base;
    RcDataExtra         *extra;
} RcDataNode;

typedef struct DataGroupImpl_t {
    Mutex               *lock;

    RK_S32              base_cnt;
    RK_S32              extra_cnt;

    /* status list header for recording the node */
    RcDataIndexes       indexes;

    // cache for current update
    RcDataNode          *curr_node;

    NodeGroup           *node;
    NodeGroup           *extra;
} DataGroupImpl;

#ifdef __cplusplus
extern "C" {
#endif

/* mpp rate control data internal update function */
MPP_RET rc_data_set_frm_status(RcDataNode *node, EncFrmStatus status);
MPP_RET rc_data_set_hal_set(RcDataNode *node, RcHalSet *set);
MPP_RET rc_data_set_hal_ret(RcDataNode *node, RcHalRet *ret);

MPP_RET rc_data_group_init(DataGroupImpl *p, RK_S32 base_cnt, RK_S32 extra_cnt);
MPP_RET rc_data_group_deinit(DataGroupImpl *p);
MPP_RET rc_data_group_reset(DataGroupImpl *p);

RcDataNode *rc_data_group_get_node_by_seq_id(DataGroupImpl *p, RK_S32 seq_id);
RcDataNode *rc_data_group_get_node_by_status(DataGroupImpl *p, RcDataStatus status, RK_S32 head);
/* put the node to the tail of next status */
void rc_data_group_put_node(DataGroupImpl *p, RcDataNode *node);

/*
 * rc data access helper function
 *
 * rc_data_get_next
 * Get next rc data for current frame rc data storage
 * equal to rc_data_group_get_node_by_status( unused, head )
 *
 * rc_data_get_curr_latest
 * Get latest current encoding frame rc data storage.
 * This function is for normal usage and reencoding.
 * equal to rc_data_group_get_node_by_status( filling, tail )
 *
 * rc_data_get_curr_tail
 * Get oldest current encoding frame rc data storage.
 * equal to rc_data_group_get_node_by_status( filling, head )
 *
 * rc_data_get_last
 * Get oldest encoded frame rc data storage.
 * equal to rc_data_group_get_node_by_status( done, head )
 *
 * Overall status and stage diagram
 *
 * oldest                                                       latest
 * +                                               +           +
 * |last                                           |  current  | next
 * +----------------------------------------------------------------->
 * |                     done                      |  filling  |unused
 * +head                                       tail+head   tail+head
 *
 * status transaction: unused -> filling -> done -> unused
 */
RcData rc_data_get_next(DataGroup grp);
RcData rc_data_get_curr_latest(DataGroup grp);
RcData rc_data_get_curr_oldest(DataGroup grp);
RcData rc_data_get_last_latest(DataGroup grp);
RcData rc_data_get_last_oldest(DataGroup grp);

#ifdef __cplusplus
}
#endif

#endif /* __RC_DATA_IMPL_H__ */
