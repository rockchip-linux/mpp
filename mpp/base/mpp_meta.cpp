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

#define MODULE_TAG "mpp_meta"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_list.h"

#include "mpp_meta_impl.h"

static MppMetaDef meta_defs[] = {
    /* categorized by type */
    /* data flow type */
    {   KEY_INPUT_FRAME,       TYPE_FRAME,    },
    {   KEY_OUTPUT_FRAME,      TYPE_FRAME,    },
    {   KEY_INPUT_PACKET,      TYPE_PACKET,   },
    {   KEY_OUTPUT_PACKET,     TYPE_PACKET,   },
    /* buffer for motion detection */
    {   KEY_MOTION_INFO,       TYPE_BUFFER,   },
    /* buffer storing the HDR information for current frame*/
    {   KEY_HDR_INFO,          TYPE_BUFFER,   },

    {   KEY_OUTPUT_INTRA,      TYPE_S32,      },
    {   KEY_INPUT_BLOCK,       TYPE_S32,      },
    {   KEY_OUTPUT_BLOCK,      TYPE_S32,      },
    {   KEY_TEMPORAL_ID,       TYPE_S32,      },
};

class MppMetaService
{
private:
    // avoid any unwanted function
    MppMetaService();
    ~MppMetaService();
    MppMetaService(const MppMetaService &);
    MppMetaService &operator=(const MppMetaService &);

    struct list_head    mlist_meta;
    struct list_head    mlist_node;

    RK_U32              meta_id;
    RK_U32              meta_count;
    RK_U32              node_count;

public:
    static MppMetaService *get_instance() {
        static MppMetaService instance;
        return &instance;
    }
    static Mutex *get_lock() {
        static Mutex lock;
        return &lock;
    }

    /*
     * get_index_of_key does two things:
     * 1. Check the key / type pair is correct or not.
     *    If failed on check return negative value
     * 2. Compare all exsisting meta data defines to find the non-negative index
     */
    RK_S32 get_index_of_key(MppMetaKey key, MppMetaType type);

    MppMetaImpl  *get_meta(const char *tag, const char *caller);
    void          put_meta(MppMetaImpl *meta);

    MppMetaNode  *get_node(MppMetaImpl *meta, RK_S32 index);
    void          put_node(MppMetaNode *node);
    MppMetaNode  *find_node(MppMetaImpl *meta, RK_S32 index);
    MppMetaNode  *next_node(MppMetaImpl *meta);
};

MppMetaService::MppMetaService()
    : meta_id(0),
      meta_count(0),
      node_count(0)
{
    INIT_LIST_HEAD(&mlist_meta);
    INIT_LIST_HEAD(&mlist_node);
}

MppMetaService::~MppMetaService()
{
    mpp_assert(list_empty(&mlist_meta));
    mpp_assert(list_empty(&mlist_node));

    while (!list_empty(&mlist_meta)) {
        MppMetaImpl *pos, *n;
        list_for_each_entry_safe(pos, n, &mlist_meta, MppMetaImpl, list_meta) {
            put_meta(pos);
        }
    }

    mpp_assert(list_empty(&mlist_node));

    while (!list_empty(&mlist_node)) {
        MppMetaNode *pos, *n;
        list_for_each_entry_safe(pos, n, &mlist_node, MppMetaNode, list_node) {
            put_node(pos);
        }
    }
}

RK_S32 MppMetaService::get_index_of_key(MppMetaKey key, MppMetaType type)
{
    RK_S32 i = 0;
    RK_S32 num = MPP_ARRAY_ELEMS(meta_defs);

    for (i = 0; i < num; i++) {
        if ((meta_defs[i].key == key) && (meta_defs[i].type == type))
            break;
    }

    return (i < num) ? (i) : (-1);
}

MppMetaImpl *MppMetaService::get_meta(const char *tag, const char *caller)
{
    MppMetaImpl *impl = mpp_malloc(MppMetaImpl, 1);
    if (impl) {
        const char *tag_src = (tag) ? (tag) : (MODULE_TAG);
        strncpy(impl->tag, tag_src, sizeof(impl->tag));
        impl->caller = caller;
        impl->meta_id = meta_id++;
        INIT_LIST_HEAD(&impl->list_meta);
        INIT_LIST_HEAD(&impl->list_node);
        impl->node_count = 0;

        list_add_tail(&impl->list_meta, &mlist_meta);
        meta_count++;
    } else {
        mpp_err_f("failed to malloc meta data\n");
    }
    return impl;
}

void MppMetaService::put_meta(MppMetaImpl *meta)
{
    while (!list_empty(&meta->list_node)) {
        MppMetaNode *node = list_entry(meta->list_node.next, MppMetaNode, list_meta);
        put_node(node);
    }
    mpp_assert(meta->node_count == 0);
    list_del_init(&meta->list_meta);
    meta_count--;
    mpp_free(meta);
}

MppMetaNode *MppMetaService::find_node(MppMetaImpl *meta, RK_S32 type_id)
{
    MppMetaNode *node = NULL;
    if (meta->node_count) {
        MppMetaNode *n, *pos;

        list_for_each_entry_safe(pos, n, &meta->list_node, MppMetaNode, list_meta) {
            if (pos->type_id == type_id) {
                node = pos;
                break;
            }
        }
    }
    return node;
}

MppMetaNode *MppMetaService::next_node(MppMetaImpl *meta)
{
    MppMetaNode *node = NULL;
    if (meta->node_count) {
        node = list_entry(meta->list_node.next, MppMetaNode, list_meta);

        list_del_init(&node->list_meta);
        list_del_init(&node->list_node);
        meta->node_count--;
        node_count--;
    }
    return node;
}

MppMetaNode *MppMetaService::get_node(MppMetaImpl *meta, RK_S32 type_id)
{
    MppMetaNode *node = find_node(meta, type_id);

    if (NULL == node) {
        node = mpp_malloc(MppMetaNode, 1);
        if (node) {
            INIT_LIST_HEAD(&node->list_meta);
            INIT_LIST_HEAD(&node->list_node);
            node->meta = meta;
            node->node_id = meta->meta_id++;
            node->type_id = type_id;
            memset(&node->val, 0, sizeof(node->val));

            meta->node_count++;
            list_add_tail(&node->list_meta, &meta->list_node);
            list_add_tail(&node->list_node, &mlist_node);
            node_count++;
        } else {
            mpp_err_f("failed to malloc meta data node\n");
        }
    }

    return node;
}

void MppMetaService::put_node(MppMetaNode *node)
{
    MppMetaImpl *meta = node->meta;
    list_del_init(&node->list_meta);
    list_del_init(&node->list_node);
    meta->node_count--;
    node_count--;
    // TODO: may be we need to release MppFrame / MppPacket / MppBuffer here
    switch (meta_defs[node->type_id].type) {
    case TYPE_FRAME : {
        // mpp_frame_deinit(&node->val.frame);
    } break;
    case TYPE_PACKET : {
        // mpp_packet_deinit(&node->val.packet);
    } break;
    case TYPE_BUFFER : {
        //mpp_buffer_put(node->val.buffer);
    } break;
    default : {
    } break;
    }
    mpp_free(node);
}

MPP_RET mpp_meta_get_with_tag(MppMeta *meta, const char *tag, const char *caller)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaService *service = MppMetaService::get_instance();
    AutoMutex auto_lock(service->get_lock());
    MppMetaImpl *impl = service->get_meta(tag, caller);
    *meta = (MppMeta) impl;
    return (impl) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_meta_put(MppMeta meta)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaService *service = MppMetaService::get_instance();
    AutoMutex auto_lock(service->get_lock());
    MppMetaImpl *impl = (MppMetaImpl *)meta;
    service->put_meta(impl);
    return MPP_OK;
}

RK_S32 mpp_meta_size(MppMeta meta)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return -1;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;

    return impl->node_count;
}

MppMetaNode *mpp_meta_next_node(MppMeta meta)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return NULL;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaService *service = MppMetaService::get_instance();
    AutoMutex auto_lock(service->get_lock());
    MppMetaNode *node = service->next_node(impl);

    return node;
}

static MPP_RET set_val_by_key(MppMetaImpl *meta, MppMetaKey key, MppMetaType type, MppMetaVal *val)
{
    MPP_RET ret = MPP_NOK;
    MppMetaService *service = MppMetaService::get_instance();
    AutoMutex auto_lock(service->get_lock());
    RK_S32 index = service->get_index_of_key(key, type);
    if (index < 0)
        return ret;

    MppMetaNode *node = service->get_node(meta, index);
    if (node) {
        node->val = *val;
        ret = MPP_OK;
    }
    return ret;
}

static MPP_RET get_val_by_key(MppMetaImpl *meta, MppMetaKey key, MppMetaType type, MppMetaVal *val)
{
    MPP_RET ret = MPP_NOK;
    MppMetaService *service = MppMetaService::get_instance();
    AutoMutex auto_lock(service->get_lock());
    RK_S32 index = service->get_index_of_key(key, type);
    if (index < 0)
        return ret;

    MppMetaNode *node = service->find_node(meta, index);
    if (node) {
        *val = node->val;
        service->put_node(node);
        ret = MPP_OK;
    }
    return ret;
}

MPP_RET mpp_meta_set_s32(MppMeta meta, MppMetaKey key, RK_S32 val)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    meta_val.val_s32 = val;
    return set_val_by_key(impl, key, TYPE_S32, &meta_val);
}

MPP_RET mpp_meta_set_s64(MppMeta meta, MppMetaKey key, RK_S64 val)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    meta_val.val_s64 = val;
    return set_val_by_key(impl, key, TYPE_S64, &meta_val);
}

MPP_RET mpp_meta_set_ptr(MppMeta meta, MppMetaKey key, void *val)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    meta_val.val_ptr = val;
    return set_val_by_key(impl, key, TYPE_PTR, &meta_val);
}

MPP_RET mpp_meta_get_s32(MppMeta meta, MppMetaKey key, RK_S32 *val)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    MPP_RET ret = get_val_by_key(impl, key, TYPE_S32, &meta_val);
    if (MPP_OK == ret)
        *val = meta_val.val_s32;

    return ret;
}

MPP_RET mpp_meta_get_s64(MppMeta meta, MppMetaKey key, RK_S64 *val)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    MPP_RET ret = get_val_by_key(impl, key, TYPE_S64, &meta_val);
    if (MPP_OK == ret)
        *val = meta_val.val_s64;

    return ret;
}

MPP_RET mpp_meta_get_ptr(MppMeta meta, MppMetaKey key, void  **val)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    MPP_RET ret = get_val_by_key(impl, key, TYPE_PTR, &meta_val);
    if (MPP_OK == ret)
        *val = meta_val.val_ptr;

    return ret;
}

MPP_RET mpp_meta_set_frame(MppMeta meta, MppMetaKey key, MppFrame frame)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    meta_val.frame = frame;
    return set_val_by_key(impl, key, TYPE_FRAME, &meta_val);
}

MPP_RET mpp_meta_set_packet(MppMeta meta, MppMetaKey key, MppPacket packet)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    meta_val.packet = packet;
    return set_val_by_key(impl, key, TYPE_PACKET, &meta_val);
}

MPP_RET mpp_meta_set_buffer(MppMeta meta, MppMetaKey key, MppBuffer buffer)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    meta_val.buffer = buffer;
    return set_val_by_key(impl, key, TYPE_BUFFER, &meta_val);
}

MPP_RET mpp_meta_get_frame(MppMeta meta, MppMetaKey key, MppFrame *frame)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    MPP_RET ret = get_val_by_key(impl, key, TYPE_FRAME, &meta_val);
    if (MPP_OK == ret)
        *frame = meta_val.frame;

    return ret;
}

MPP_RET mpp_meta_get_packet(MppMeta meta, MppMetaKey key, MppPacket *packet)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    MPP_RET ret = get_val_by_key(impl, key, TYPE_PACKET, &meta_val);
    if (MPP_OK == ret)
        *packet = meta_val.packet;

    return ret;
}

MPP_RET mpp_meta_get_buffer(MppMeta meta, MppMetaKey key, MppBuffer *buffer)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppMetaVal meta_val;
    MPP_RET ret = get_val_by_key(impl, key, TYPE_BUFFER, &meta_val);
    if (MPP_OK == ret)
        *buffer = meta_val.buffer;

    return ret;
}

