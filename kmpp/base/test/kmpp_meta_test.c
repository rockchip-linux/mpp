/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_meta_test"

#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_thread.h"

#include "rk_venc_cmd.h"

#include "kmpp_obj.h"
#include "kmpp_meta_impl.h"
#include "kmpp_venc_utils.h"

#define THRD_DEFAULT    4
#define LOOP_DEFAULT    10
#define META_TEST_USERDATA_LEN  (4 * 1024)

/* oversized userdata buffer to force kmpp_obj_resize (shm realloc) */
typedef struct {
    MppThread       *thread;
    RK_S32          loop_cnt;
    RK_S64          time_avg;
    rk_u8           *ud_data;
} MetaTestCtx;

static MPP_RET test_user_datas_uuid(void)
{
    static const rk_u8 uuid[MPP_ENC_USER_DATA_UUID_LEN] = {
        0x66, 0x72, 0x6d, 0x00, 0x63, 0x66, 0x67, 0x2d,
        0x75, 0x64, 0x73, 0x00, 0xa5, 0x5a, 0x19, 0x26,
    };
    static const rk_u8 payload[] = "kmpp-meta-uuid";
    MPP_ENC_UDS1(src);
    MppEncUserDataSetShm *get = NULL;
    KmppMeta meta = NULL;
    MPP_RET ret;

    memset(&src, 0, sizeof(src));
    src.set.count = 1;
    src.set.data[0].len = sizeof(payload) - 1;
    src.set.data[0].uuid.uptr = (rk_u8 *)uuid;
    src.set.data[0].data.uptr = (rk_u8 *)payload;

    ret = kmpp_meta_get_f(&meta);
    if (ret)
        return ret;

    ret = kmpp_meta_set_ptr(meta, KEY_USER_DATAS, &src.set);
    if (!ret)
        ret = kmpp_meta_get_ptr(meta, KEY_USER_DATAS, (void **)&get);

    if (!ret && (!get || get->count != 1 ||
                 !get->data[0].uuid.uptr || !get->data[0].data.uptr ||
                 get->data[0].len != src.set.data[0].len ||
                 memcmp(get->data[0].uuid.uptr, uuid, sizeof(uuid)) ||
                 memcmp(get->data[0].data.uptr, payload, src.set.data[0].len)))
        ret = MPP_NOK;

    kmpp_meta_put_f(meta);

    return ret;
}

static MPP_RET meta_set(KmppMeta meta, rk_u8 *ud_data)
{
    KmppShmPtr zero = {{0}, {0}};
    MppEncUserDataShm ud = {0};
    MppEncUserDataSetShm uds = {0};
    MPP_RET ret = MPP_OK;

    /* oversized userdata forces kmpp_obj_resize (shm realloc) on set_ptr */
    ud.data.uptr = ud_data;
    ud.len = ud_data ? META_TEST_USERDATA_LEN : 0;

    ret |= kmpp_meta_set_shm(meta, KEY_INPUT_FRAME, &zero);
    ret |= kmpp_meta_set_shm(meta, KEY_INPUT_PACKET, &zero);
    ret |= kmpp_meta_set_shm(meta, KEY_OUTPUT_FRAME, &zero);
    ret |= kmpp_meta_set_shm(meta, KEY_OUTPUT_PACKET, &zero);

    ret |= kmpp_meta_set_shm(meta, KEY_MOTION_INFO, &zero);
    ret |= kmpp_meta_set_shm(meta, KEY_HDR_INFO, &zero);

    ret |= kmpp_meta_set_s32(meta, KEY_INPUT_BLOCK, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_OUTPUT_BLOCK, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_INPUT_IDR_REQ, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_INPUT_PSKIP_NUM, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_OUTPUT_INTRA, 0);

    ret |= kmpp_meta_set_s32(meta, KEY_TEMPORAL_ID, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_LONG_REF_IDX, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_ENC_AVERAGE_QP, 0);

    //ret |= kmpp_meta_set_shm(meta, KEY_ROI_DATA, NULL);
    ret |= kmpp_meta_set_shm(meta, KEY_OSD_DATA, NULL);
    ret |= kmpp_meta_set_shm(meta, KEY_OSD_DATA2, NULL);
    ret |= kmpp_meta_set_shm(meta, KEY_OSD_DATA3, NULL);
    ret |= kmpp_meta_set_ptr(meta, KEY_USER_DATA, &ud);
    ret |= kmpp_meta_set_ptr(meta, KEY_USER_DATAS, &uds);

    ret |= kmpp_meta_set_shm(meta, KEY_QPMAP0, NULL);
    ret |= kmpp_meta_set_shm(meta, KEY_NPU_SOBJ_FLAG, NULL);
    ret |= kmpp_meta_set_ptr(meta, KEY_NPU_UOBJ_FLAG, NULL);

    ret |= kmpp_meta_set_s32(meta, KEY_ENC_MARK_LTR, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_ENC_USE_LTR, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_ENC_FRAME_QP, 0);
    ret |= kmpp_meta_set_s32(meta, KEY_ENC_BASE_LAYER_PID, 0);

    return ret;
}

static MPP_RET meta_get(KmppMeta meta)
{
    KmppShmPtr shm;
    void *ptr;
    RK_S32 val;
    MPP_RET ret = MPP_OK;

    ret |= kmpp_meta_get_shm(meta, KEY_INPUT_FRAME, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_INPUT_PACKET, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_OUTPUT_FRAME, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_OUTPUT_PACKET, &shm);

    ret |= kmpp_meta_get_shm(meta, KEY_MOTION_INFO, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_HDR_INFO, &shm);

    ret |= kmpp_meta_get_s32(meta, KEY_INPUT_BLOCK, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_OUTPUT_BLOCK, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_INPUT_IDR_REQ, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_INPUT_PSKIP_NUM, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_OUTPUT_INTRA, &val);

    ret |= kmpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &val);

    //ret |= kmpp_meta_get_shm(meta, KEY_ROI_DATA, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_OSD_DATA, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_OSD_DATA2, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_OSD_DATA3, &shm);
    ret |= kmpp_meta_get_ptr(meta, KEY_USER_DATA, &ptr);
    if (ptr) {
        MppEncUserDataShm *shm_ud = (MppEncUserDataShm *)ptr;

        if (shm_ud->len != (RK_U32)META_TEST_USERDATA_LEN)
            mpp_log("USER_DATA len mismatch %u != %d\n",
                    shm_ud->len, META_TEST_USERDATA_LEN);
    }
    ret |= kmpp_meta_get_ptr(meta, KEY_USER_DATAS, &ptr);

    ret |= kmpp_meta_get_shm(meta, KEY_QPMAP0, &shm);
    ret |= kmpp_meta_get_shm(meta, KEY_NPU_SOBJ_FLAG, &shm);
    ret |= kmpp_meta_get_ptr(meta, KEY_NPU_UOBJ_FLAG, &ptr);

    ret |= kmpp_meta_get_s32(meta, KEY_ENC_MARK_LTR, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_ENC_USE_LTR, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_ENC_FRAME_QP, &val);
    ret |= kmpp_meta_get_s32(meta, KEY_ENC_BASE_LAYER_PID, &val);

    return ret;
}

/*
 * Verify resize rebind correctness:
 *   1. priv_offset: get_by_sptr(meta sptr) returns the ORIGINAL meta impl
 *      (not the transient new_obj head)
 *   2. old values survive resize (shm data migration correct)
 *   3. new flex space is accessible and inline data is intact
 */
static void test_resize_rebind(rk_u8 *ud_data)
{
    KmppMeta meta = NULL;
    MppEncUserDataShm ud = {0};
    KmppShmPtr *sptr;
    KmppObj obj;
    RK_S32 qp = -1;
    void *ptr = NULL;

    mpp_log(MODULE_TAG " test_resize_rebind start\n");

    kmpp_meta_get_f(&meta);
    mpp_assert(meta);

    /* a normal s32 key set before resize — must survive */
    kmpp_meta_set_s32(meta, KEY_ENC_AVERAGE_QP, 42);

    /* oversized userdata forces resize → shm realloc + data migration */
    ud.data.uptr = ud_data;
    ud.len = ud_data ? META_TEST_USERDATA_LEN : 0;
    if (kmpp_meta_set_ptr(meta, KEY_USER_DATA, &ud)) {
        mpp_log(MODULE_TAG " set_ptr USER_DATA failed\n");
        kmpp_meta_put_f(meta);
        return;
    }

    /* 1. priv_offset: get_by_sptr must resolve to the original meta */
    sptr = kmpp_obj_to_shm((KmppObj)meta);
    kmpp_obj_get_by_sptr_f(&obj, sptr);
    if (obj != (KmppObj)meta) {
        mpp_log(MODULE_TAG " REBIND FAILED: obj %p != meta %p\n", obj, meta);
        kmpp_meta_put_f(meta);
        return;
    }

    /* 2. old value survived resize (s32 migrated correctly) */
    kmpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &qp);
    if (qp != 42)
        mpp_log(MODULE_TAG " old s32 lost after resize: %d != 42\n", qp);

    /* 3. new flex space: USER_DATA inline header + data intact */
    kmpp_meta_get_ptr(meta, KEY_USER_DATA, &ptr);
    if (ptr) {
        MppEncUserDataShm *shm_ud = (MppEncUserDataShm *)ptr;

        if (shm_ud->len != (RK_U32)ud.len)
            mpp_log(MODULE_TAG " USER_DATA len mismatch %u != %u\n",
                    shm_ud->len, ud.len);
        else if (memcmp(shm_ud->data.uptr, ud_data, ud.len))
            mpp_log(MODULE_TAG " USER_DATA data corrupted after resize\n");
    }

    mpp_log(MODULE_TAG " resize rebind OK\n");
    kmpp_meta_put_f(meta);
}

/*
 * Review fix: setting USER_DATAS before USER_DATA must not overlap.
 * USER_DATA sits at FLEX_FIXED_SIZE; USER_DATAS must be shifted past it.
 */
static void test_flex_order(rk_u8 *ud_data)
{
    MppEncUserDataSetShm *uds = mpp_calloc_size(MppEncUserDataSetShm,
                                                sizeof(MppEncUserDataSetShm) +
                                                sizeof(MppEncUserDataFullShm));
    KmppMeta meta = NULL;
    MppEncUserDataShm ud = {0};
    rk_u8 data_buf[64];
    rk_u8 uuid_buf[16] = "uuid-0";
    void *ptr = NULL;

    mpp_log(MODULE_TAG " test_flex_order start\n");

    kmpp_meta_get_f(&meta);
    mpp_assert(meta);

    memset(data_buf, 0xCC, sizeof(data_buf));
    uds->count = 1;
    uds->data[0].len = sizeof(data_buf);
    uds->data[0].uuid.uptr = uuid_buf;
    uds->data[0].data.uptr = data_buf;

    /* USER_DATAS first, then USER_DATA — must not overwrite each other */
    {
        rk_s32 sret = kmpp_meta_set_ptr(meta, KEY_USER_DATAS, uds);
        mpp_free(uds);
        if (sret) {
            mpp_log(MODULE_TAG " flex_order: set USER_DATAS failed\n");
            kmpp_meta_put_f(meta);
            return;
        }
    }

    ud.data.uptr = ud_data;
    ud.len = ud_data ? META_TEST_USERDATA_LEN : 0;
    if (kmpp_meta_set_ptr(meta, KEY_USER_DATA, &ud)) {
        mpp_log(MODULE_TAG " flex_order: set USER_DATA failed\n");
        kmpp_meta_put_f(meta);
        return;
    }

    kmpp_meta_get_ptr(meta, KEY_USER_DATAS, &ptr);
    if (ptr) {
        MppEncUserDataSetShm *shm_uds = (MppEncUserDataSetShm *)ptr;

        if (shm_uds->count != 1)
            mpp_log(MODULE_TAG " flex_order: USR_DATAS count corrupted %u\n", shm_uds->count);
        else if (shm_uds->data[0].len != sizeof(data_buf))
            mpp_log(MODULE_TAG " flex_order: USR_DATAS len corrupted\n");
        else if (memcmp(shm_uds->data[0].data.uptr, data_buf, sizeof(data_buf)))
            mpp_log(MODULE_TAG " flex_order: USR_DATAS data corrupted by USER_DATA\n");
    }

    kmpp_meta_get_ptr(meta, KEY_USER_DATA, &ptr);
    if (ptr) {
        MppEncUserDataShm *shm_ud = (MppEncUserDataShm *)ptr;

        if (shm_ud->len != (RK_U32)ud.len)
            mpp_log(MODULE_TAG " flex_order: USER_DATA len mismatch\n");
        else if (memcmp(shm_ud->data.uptr, ud_data, ud.len))
            mpp_log(MODULE_TAG " flex_order: USER_DATA data corrupted\n");
    }

    mpp_log(MODULE_TAG " flex_order OK\n");
    kmpp_meta_put_f(meta);
}

/*
 * Review fix: growing USER_DATA after USER_DATAS is set must shift USER_DATAS
 * based on length (not capacity), else USER_DATA overwrites USER_DATAS.
 */
static void test_flex_grow(rk_u8 *ud_data)
{
    MppEncUserDataSetShm *uds = mpp_calloc_size(MppEncUserDataSetShm,
                                                sizeof(MppEncUserDataSetShm) +
                                                sizeof(MppEncUserDataFullShm));
    KmppMeta meta = NULL;
    MppEncUserDataShm ud = {0};
    rk_u8 uds_buf[64];
    rk_u8 small_buf[64];
    void *ptr = NULL;

    mpp_log(MODULE_TAG " test_flex_grow start\n");

    kmpp_meta_get_f(&meta);
    mpp_assert(meta);

    /* small USER_DATA first */
    memset(small_buf, 0xDD, sizeof(small_buf));
    ud.data.uptr = small_buf;
    ud.len = sizeof(small_buf);
    kmpp_meta_set_ptr(meta, KEY_USER_DATA, &ud);

    /* USER_DATAS after USER_DATA */
    memset(uds_buf, 0xCC, sizeof(uds_buf));
    uds->count = 1;
    uds->data[0].len = sizeof(uds_buf);
    uds->data[0].uuid.uptr = NULL;
    uds->data[0].data.uptr = uds_buf;
    kmpp_meta_set_ptr(meta, KEY_USER_DATAS, uds);
    mpp_free(uds);

    /* grow USER_DATA — must shift USER_DATAS, not overwrite it */
    ud.data.uptr = ud_data;
    ud.len = ud_data ? META_TEST_USERDATA_LEN : 0;
    kmpp_meta_set_ptr(meta, KEY_USER_DATA, &ud);

    /* verify USER_DATAS survived the grow */
    kmpp_meta_get_ptr(meta, KEY_USER_DATAS, &ptr);
    if (ptr) {
        MppEncUserDataSetShm *shm_uds = (MppEncUserDataSetShm *)ptr;

        if (shm_uds->count != 1)
            mpp_log(MODULE_TAG " flex_grow: USR_DATAS count corrupted\n");
        else if (shm_uds->data[0].len != sizeof(uds_buf))
            mpp_log(MODULE_TAG " flex_grow: USR_DATAS len corrupted\n");
        else if (memcmp(shm_uds->data[0].data.uptr, uds_buf, sizeof(uds_buf)))
            mpp_log(MODULE_TAG " flex_grow: USR_DATAS data corrupted by USER_DATA grow\n");
    }

    /* verify grown USER_DATA intact */
    kmpp_meta_get_ptr(meta, KEY_USER_DATA, &ptr);
    if (ptr) {
        MppEncUserDataShm *shm_ud = (MppEncUserDataShm *)ptr;

        if (shm_ud->len != (RK_U32)ud.len)
            mpp_log(MODULE_TAG " flex_grow: USER_DATA len mismatch\n");
        else if (memcmp(shm_ud->data.uptr, ud_data, ud.len))
            mpp_log(MODULE_TAG " flex_grow: USER_DATA data corrupted\n");
    }

    mpp_log(MODULE_TAG " flex_grow OK\n");
    kmpp_meta_put_f(meta);
}

/* set a FIXED-flex key (OSD_DATA4, copy-into-meta mode) before any variable
 * key — exercises on-demand FIX section allocation in set_ptr. */
static void test_fixed_only(void)
{
    KmppMeta meta = NULL;
    MppEncOSDData3 osd;
    MppEncOSDData3 *get;
    void *ptr = NULL;

    mpp_log(MODULE_TAG " test_fixed_only start\n");

    kmpp_meta_get_f(&meta);
    mpp_assert(meta);

    memset(&osd, 0, sizeof(osd));
    osd.change = 1;
    osd.num_region = 1;

    if (kmpp_meta_set_ptr(meta, KEY_OSD_DATA4, &osd)) {
        mpp_log(MODULE_TAG " fixed_only: set OSD_DATA4 failed\n");
        kmpp_meta_put_f(meta);
        return;
    }

    kmpp_meta_get_ptr(meta, KEY_OSD_DATA4, &ptr);
    if (ptr) {
        get = (MppEncOSDData3 *)ptr;

        if (get->num_region != osd.num_region)
            mpp_log(MODULE_TAG " fixed_only: OSD_DATA4 num_region mismatch %u != %u\n",
                    get->num_region, osd.num_region);
        else
            mpp_log(MODULE_TAG " fixed_only: OSD_DATA4 verified\n");
    } else {
        mpp_log(MODULE_TAG " fixed_only: OSD_DATA4 get_ptr NULL\n");
    }

    mpp_log(MODULE_TAG " fixed_only OK\n");
    kmpp_meta_put_f(meta);
}

void *meta_test(void *param)
{
    MetaTestCtx *ctx = (MetaTestCtx *)param;
    RK_S32 loop_max = ctx->loop_cnt;
    RK_S64 time_start;
    RK_S64 time_end;
    MPP_RET ret = MPP_OK;
    RK_S32 i;

    time_start = mpp_time();

    for (i = 0; i < loop_max; i++) {
        KmppMeta meta = NULL;

        ret |= kmpp_meta_get_f(&meta);
        mpp_assert(meta);

        /* set */
        ret |= meta_set(meta, ctx->ud_data);
        /* get */
        ret |= meta_get(meta);

        ret |= kmpp_meta_put_f(meta);
    }

    time_end = mpp_time();

    if (ret)
        mpp_log("meta setting and getting, ret %d\n", ret);

    ctx->time_avg = (time_end - time_start) / loop_max;

    return NULL;
}

int main(int argc, char **argv)
{
    MetaTestCtx *ctxs = NULL;
    RK_S32 loop_cnt = 0;
    RK_S32 thd_cnt = 0;
    RK_S32 created = 0;
    RK_S64 avg_time = 0;
    MPP_RET ret = MPP_OK;
    RK_S32 i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-loop") && i + 1 < argc)
            loop_cnt = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-threads") && i + 1 < argc)
            thd_cnt = atoi(argv[++i]);
    }

    if (loop_cnt <= 0)
        loop_cnt = LOOP_DEFAULT;
    if (thd_cnt <= 0)
        thd_cnt = THRD_DEFAULT;

    /* allocate ctxs + one shared ud_data buffer (appended after the array) */
    ctxs = mpp_calloc_size(MetaTestCtx, sizeof(MetaTestCtx) * thd_cnt +
                           META_TEST_USERDATA_LEN);
    if (!ctxs) {
        mpp_log(MODULE_TAG " alloc failed\n");
        ret = MPP_NOK;
        goto done;
    }

    {
        rk_u8 *ud_buf = (rk_u8 *)(ctxs + thd_cnt);

        memset(ud_buf, 0xAB, META_TEST_USERDATA_LEN);
        for (i = 0; i < thd_cnt; i++)
            ctxs[i].ud_data = ud_buf;
    }

    test_resize_rebind(ctxs[0].ud_data);

    ret = test_user_datas_uuid();
    if (ret) {
        mpp_loge(MODULE_TAG " binary uuid failed ret %d\n", ret);
        goto done;
    }

    test_flex_order(ctxs[0].ud_data);
    test_flex_grow(ctxs[0].ud_data);
    test_fixed_only();

    mpp_log(MODULE_TAG " start threads %d loop %d\n", thd_cnt, loop_cnt);

    for (i = 0; i < thd_cnt; i++) {
        ctxs[i].loop_cnt = loop_cnt;
        ctxs[i].thread = mpp_thread_create(meta_test, &ctxs[i], "meta_test");
        if (!ctxs[i].thread) {
            mpp_log(MODULE_TAG " thread %d create failed\n", i);
            ret = MPP_NOK;
            goto done;
        }
        mpp_thread_start(ctxs[i].thread);
        created++;
    }

done:
    /* join + destroy all successfully created threads */
    for (i = 0; i < created; i++) {
        mpp_thread_destroy(ctxs[i].thread);
        ctxs[i].thread = NULL;
    }

    if (ret == MPP_OK) {
        for (i = 0; i < created; i++)
            avg_time += ctxs[i].time_avg;
        mpp_log(MODULE_TAG " %d threads %d loop config avg %lld us",
                created, loop_cnt, created > 0 ? avg_time / created : 0);
        mpp_log(MODULE_TAG " done\n");
    }

    MPP_FREE(ctxs);

    return ret;
}
