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

#ifndef __MPP_META_IMPL_H__
#define __MPP_META_IMPL_H__

#include "mpp_common.h"

#include "mpp_meta.h"

typedef struct MppMetaDef_t {
    MppMetaKey          key;
    MppMetaType         type;
} MppMetaDef;

typedef struct MppMetaImpl_t {
    char                tag[MPP_TAG_SIZE];
    const char          *caller;
    RK_S32              meta_id;

    struct list_head    list_meta;
    struct list_head    list_node;
    RK_S32              node_count;
} MppMetaImpl;

typedef union MppMetaVal_u {
    RK_S32              val_s32;
    RK_S64              val_s64;
    void                *val_ptr;
    MppFrame            frame;
    MppPacket           packet;
    MppBuffer           buffer;
} MppMetaVal;

typedef struct MppMetaNode_t {
    struct list_head    list_meta;
    struct list_head    list_node;
    MppMetaImpl         *meta;
    RK_S32              node_id;

    RK_S32              type_id;
    MppMetaVal          val;
} MppMetaNode;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpp_meta_size(MppMeta meta);
MppMetaNode *mpp_meta_next_node(MppMeta meta);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_META_IMPL_H__*/
