/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_server"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_list.h"
#include "mpp_time.h"
#include "osal_2str.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_mem_pool.h"
#include "mpp_singleton.h"

#include "mpp_server.h"
#include "mpp_device_debug.h"
#include "mpp_service_impl.h"

#define MAX_BATCH_TASK      8
#define MAX_SESSION_TASK    4
#define MAX_REQ_SEND_CNT    MAX_REQ_NUM
#define MAX_REQ_WAIT_CNT    2

#define MPP_SERVER_DBG_FLOW             (0x00000001)

#define mpp_serv_dbg(flag, fmt, ...)    _mpp_dbg(mpp_server_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_serv_dbg_f(flag, fmt, ...)  _mpp_dbg_f(mpp_server_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_serv_dbg_flow(fmt, ...)     mpp_serv_dbg(MPP_SERVER_DBG_FLOW, fmt, ## __VA_ARGS__)

#define FIFO_WRITE(size, count, wr, rd) \
    do { \
        wr++; \
        if (wr >= size) wr = 0; \
        count++; \
        mpp_assert(count <= size); \
    } while (0)

#define FIFO_READ(size, count, wr, rd) \
    do { \
        rd++; \
        if (rd >= size) rd = 0; \
        count--; \
        mpp_assert(count >= 0); \
    } while (0)

#define get_srv_server() \
    ({ \
        MppDevServer *__tmp; \
        if (!srv_server) { \
            mpp_server_init(); \
        } \
        __tmp = srv_server; \
        if (!__tmp || !__tmp->inited) { \
            mpp_err("mpp server srv not init for %s at %s\n", \
                    __tmp ? __tmp->server_error : "invalid server", __FUNCTION__); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

typedef struct MppDevTask_t     MppDevTask;
typedef struct MppDevBatTask_t  MppDevBatTask;
typedef struct MppDevSession_t  MppDevSession;
typedef struct MppDevBatServ_t  MppDevBatServ;

struct MppDevTask_t {
    /* link to server */
    struct list_head    link_server;
    /* link to session tasks */
    struct list_head    link_session;
    /* link to batch tasks */
    struct list_head    link_batch;

    MppDevSession       *session;
    MppDevBatTask       *batch;

    rk_s32              slot_idx;

    /* lock by server */
    rk_s32              task_id;
    /* lock by batch */
    rk_s32              batch_slot_id;

    MppReqV1            *req;
    rk_s32              req_cnt;
};

struct MppDevBatTask_t {
    MppMutexCond        cond;

    /* link to server */
    struct list_head    link_server;
    /* link to session tasks */
    struct list_head    link_tasks;

    rk_u32              batch_id;

    MppDevBatCmd        *bat_cmd;
    MppReqV1            *send_reqs;
    MppReqV1            *wait_reqs;

    /* lock and clean by server */
    rk_s32              send_req_cnt;
    rk_s32              wait_req_cnt;

    /* lock by server */
    rk_s32              fill_cnt;
    rk_s32              fill_full;
    rk_s32              fill_timeout;
    rk_s32              poll_cnt;
};

struct MppDevSession_t {
    MppMutexCond        cond_lock;

    /* hash table to server */
    struct list_head    list_server;
    /* link to session waiting tasks */
    struct list_head    list_wait;
    /* link to session free tasks */
    struct list_head    list_done;

    MppDevMppService    *ctx;
    MppDevBatServ       *server;

    rk_s32              client;

    rk_s32              task_wait;
    rk_s32              task_done;

    MppDevTask          tasks[MAX_SESSION_TASK];
};

struct MppDevBatServ_t {
    MppMutex            lock;

    rk_s32              server_fd;
    rk_u32              batch_id;
    rk_u32              task_id;

    /* timer for serializing process */
    MppTimer            timer;

    /* session register */
    struct list_head    session_list;
    rk_s32              session_count;

    /* batch task queue */
    struct list_head    list_batch;
    struct list_head    list_batch_free;
    MppMemPool          batch_pool;
    rk_s32              batch_max_count;
    rk_s32              batch_task_size;
    rk_s32              batch_run;
    rk_s32              batch_free;
    rk_s32              max_task_in_batch;

    /* link to all pending tasks */
    struct list_head    pending_task;
    rk_s32              pending_count;
};

typedef struct MppDevServer_t {
    const char          *server_error;
    const char          *server_name;
    rk_s32              inited;
    rk_u32              enable;
    MppMutex            lock;
    MppDevBatServ       *bat_server[VPU_CLIENT_BUTT];

    MppMemPool          session_pool;
    MppMemPool          batch_pool;

    rk_s32              max_task_in_batch;

    const MppServiceCmdCap *cmd_cap;
} MppDevServer;

static MppDevServer *srv_server = NULL;
static rk_u32 mpp_server_debug = 0;

static void batch_reset(MppDevBatTask *batch)
{
    mpp_assert(list_empty(&batch->link_tasks));

    batch->fill_cnt = 0;
    batch->fill_full = 0;
    batch->fill_timeout = 0;
    batch->poll_cnt = 0;
    batch->send_req_cnt = 0;
    batch->wait_req_cnt = 0;
}

MppDevBatTask *batch_add(MppDevBatServ *server)
{
    MppDevBatTask *batch = (MppDevBatTask *)mpp_mem_pool_get_f(server->batch_pool);

    mpp_assert(batch);
    if (!batch)
        return batch;

    INIT_LIST_HEAD(&batch->link_server);
    INIT_LIST_HEAD(&batch->link_tasks);

    batch->send_reqs = (MppReqV1 *)(batch + 1);
    batch->wait_reqs = batch->send_reqs +
                       (server->max_task_in_batch * MAX_REQ_SEND_CNT);
    batch->bat_cmd = (MppDevBatCmd *)(batch->wait_reqs +
                                      (server->max_task_in_batch * MAX_REQ_WAIT_CNT));

    batch_reset(batch);
    list_add_tail(&batch->link_server, &server->list_batch_free);
    server->batch_free++;

    mpp_serv_dbg_flow("batch add free count %d:%d\n", server->batch_run, server->batch_free);
    return batch;
}

void batch_del(MppDevBatServ *server, MppDevBatTask *batch)
{
    mpp_assert(batch);
    mpp_assert(batch->fill_cnt == 0);
    mpp_assert(list_empty(&batch->link_tasks));

    list_del_init(&batch->link_server);

    mpp_mem_pool_put_f(server->batch_pool, batch);
    server->batch_free--;
    mpp_serv_dbg_flow("batch del free count %d:%d\n", server->batch_run, server->batch_free);
}

void batch_send(MppDevBatServ *server, MppDevBatTask *batch)
{
    rk_s32 ret = 0;

    mpp_assert(batch->send_req_cnt);

    ret = mpp_service_ioctl_request(server->server_fd, batch->send_reqs);
    if (ret) {
        mpp_err_f("ioctl batch cmd failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
        mpp_serv_dbg_flow("batch %d -> send failed\n", batch->batch_id);
    }

    list_del_init(&batch->link_server);
    list_add_tail(&batch->link_server, &server->list_batch);
    server->batch_free--;
    server->batch_run++;
    mpp_serv_dbg_flow("batch %d -> send %d for %s\n", batch->batch_id,
                      batch->fill_cnt, batch->fill_timeout ? "timeout" : "ready");
}

void process_task(void *p)
{
    MppDevBatServ *server = (MppDevBatServ *)p;
    MppMutex *lock = &server->lock;
    rk_s32 ret = rk_ok;
    MppDevTask *task;
    MppDevBatTask *batch;
    MppDevSession *session = NULL;
    MppDevBatCmd *bat_cmd;
    MppReqV1 *req = NULL;
    rk_s32 pending = 0;

    mpp_serv_dbg_flow("process task start\n");

    /* 1. try poll and get finished task */
    do {
        batch = list_first_entry_or_null(&server->list_batch, MppDevBatTask, link_server);
        if (!batch)
            break;

        mpp_assert(batch->wait_req_cnt);
        ret = mpp_service_ioctl_request(server->server_fd, batch->wait_reqs);
        if (!ret) {
            MppDevTask *n;

            list_for_each_entry_safe(task, n, &batch->link_tasks, MppDevTask, link_batch) {
                rk_s32 batch_slot_id = task->batch_slot_id;
                MppDevBatCmd *cmd = batch->bat_cmd + batch_slot_id;

                mpp_assert(batch_slot_id < server->max_task_in_batch);
                session = task->session;

                ret = cmd->ret;
                if (ret == EAGAIN)
                    continue;

                if (ret == -EIO) {
                    mpp_err_f("batch %d:%d task %d poll error found\n",
                              batch->batch_id, task->batch_slot_id, task->task_id);
                    cmd->flag |= 1;
                    continue;
                }
                if (ret == 0) {
                    list_del_init(&task->link_batch);
                    task->batch = NULL;

                    mpp_serv_dbg_flow("batch %d:%d session %d ready and remove\n",
                                      batch->batch_id, task->batch_slot_id, session->client);
                    mpp_mutex_cond_lock(&session->cond_lock);
                    session->task_done++;
                    mpp_mutex_cond_signal(&session->cond_lock);
                    mpp_mutex_cond_unlock(&session->cond_lock);
                    if (session->ctx && session->ctx->dev_cb)
                        mpp_callback(session->ctx->dev_cb, NULL);

                    batch->poll_cnt++;
                    cmd->flag |= 1;
                }
            }

            mpp_serv_dbg_flow("batch %d fill %d poll %d\n", batch->batch_id,
                              batch->fill_cnt, batch->poll_cnt);

            if (batch->poll_cnt == batch->fill_cnt) {
                mpp_serv_dbg_flow("batch %d poll done\n", batch->batch_id);
                list_del_init(&batch->link_server);
                list_add_tail(&batch->link_server, &server->list_batch_free);
                server->batch_run--;
                server->batch_free++;

                batch_reset(batch);
                batch = NULL;
                continue;
            }
        } else {
            mpp_log_f("batch %d poll ret %d errno %d %s", batch->batch_id,
                      ret, errno, strerror(errno));
            mpp_log_f("stop timer\n");
            mpp_timer_set_enable(server->timer, 0);
        }
        break;
    } while (1);

    /* 2. get prending task to fill */
    mpp_mutex_lock(lock);
    pending = server->pending_count;
    if (!pending && !server->batch_run && !server->session_count) {
        mpp_timer_set_enable(server->timer, 0);
        mpp_serv_dbg_flow("stop timer\n");
    }
    mpp_mutex_unlock(lock);

    mpp_serv_dbg_flow("pending %d running %d free %d max %d process start\n",
                      pending, server->batch_run, server->batch_free, server->batch_max_count);

try_proc_pending_task:
    /* 3. try get batch task*/
    if (!server->batch_free) {
        /* if not enough and max count does not reached create new batch */
        if (server->batch_free + server->batch_run >= server->batch_max_count) {
            mpp_serv_dbg_flow("finish for not batch slot\n");
            return ;
        }

        batch_add(server);
    }
    mpp_assert(server->batch_free);

    /* 4. if no pending task to send check timeout batch and send it */
    if (!pending) {
        if (!server->batch_free) {
            /* no pending and no free batch just done */
            return;
        }

        batch = list_first_entry_or_null(&server->list_batch_free, MppDevBatTask, link_server);
        mpp_assert(batch);
        if (!batch) {
            mpp_log_f("batch run %d free %d\n", server->batch_run, server->batch_free);
            return;
        }

        /* send one timeout task */
        if (batch->fill_cnt)
            batch_send(server, batch);

        mpp_serv_dbg_flow("finish for no pending task\n");
        return;
    }

    mpp_serv_dbg_flow("pending task %d left to process\n", pending);

    /* 5. add task to add batch and try send batch */
    if (!server->batch_free) {
        /* no pending and no free batch just done */
        return;
    }

    batch = list_first_entry_or_null(&server->list_batch_free, MppDevBatTask, link_server);
    mpp_assert(batch);
    mpp_assert(pending);

    task = NULL;
    mpp_mutex_lock(lock);
    task = list_first_entry_or_null(&server->pending_task, MppDevTask, link_server);
    list_del_init(&task->link_server);
    server->pending_count--;
    mpp_mutex_unlock(lock);
    pending--;

    /* first task and setup new batch id */
    if (!batch->fill_cnt)
        batch->batch_id = server->batch_id++;

    task->batch = batch;
    task->batch_slot_id = batch->fill_cnt++;
    mpp_assert(task->batch_slot_id < server->max_task_in_batch);
    list_add_tail(&task->link_batch, &batch->link_tasks);
    if (batch->fill_cnt >= server->max_task_in_batch)
        batch->fill_full = 1;

    session = task->session;
    mpp_assert(session);
    mpp_assert(session->ctx);

    bat_cmd = batch->bat_cmd + task->batch_slot_id;
    bat_cmd->flag = 0;
    bat_cmd->client = session->client;
    bat_cmd->ret = 0;

    /* fill task to batch */
    /* add session info before each session task and then copy session request */
    req = &batch->send_reqs[batch->send_req_cnt++];
    req->cmd = MPP_CMD_SET_SESSION_FD;
    req->flag = MPP_FLAGS_MULTI_MSG;
    req->offset = 0;
    req->size = sizeof(*bat_cmd);
    req->data_ptr = REQ_DATA_PTR(bat_cmd);

    {
        rk_s32 i;

        for (i = 0; i < task->req_cnt; i++)
            batch->send_reqs[batch->send_req_cnt++] = task->req[i];
    }

    mpp_assert(batch->send_req_cnt <= server->max_task_in_batch * MAX_REQ_NUM);

    /* setup poll request */
    req = &batch->wait_reqs[batch->wait_req_cnt++];
    req->cmd = MPP_CMD_SET_SESSION_FD;
    req->flag = MPP_FLAGS_MULTI_MSG;
    req->offset = 0;
    req->size = sizeof(*bat_cmd);
    req->data_ptr = REQ_DATA_PTR(bat_cmd);

    req = &batch->wait_reqs[batch->wait_req_cnt++];
    req->cmd = MPP_CMD_POLL_HW_FINISH;
    req->flag = MPP_FLAGS_POLL_NON_BLOCK | MPP_FLAGS_MULTI_MSG | MPP_FLAGS_LAST_MSG;
    req->offset = 0;
    req->size = 0;
    req->data_ptr = 0;

    mpp_serv_dbg_flow("batch %d:%d add task %d:%d:%d\n",
                      batch->batch_id, task->batch_slot_id, session->client,
                      task->slot_idx, task->task_id);

    if (batch->fill_full) {
        list_del_init(&batch->link_server);
        list_add_tail(&batch->link_server, &server->list_batch);
        mpp_serv_dbg_flow("batch %d -> fill_nb %d fill ready\n",
                          batch->batch_id, batch->fill_cnt);
        batch_send(server, batch);
        batch = NULL;
    }
    goto try_proc_pending_task;
}

static void *mpp_server_thread(void *ctx)
{
    process_task((MppDevBatServ *)ctx);
    return NULL;
}

rk_s32 send_task(MppDevMppService *ctx)
{
    MppDevTask *task = NULL;
    MppDevSession *session = (MppDevSession *)ctx->serv_ctx;

    if (!session || !session->server) {
        mpp_err_f("invalid ctx %p session %p send task\n", ctx, session);
        return rk_nok;
    }

    MppDevBatServ *server = session->server;

    /* get free task from session and add to run list */
    mpp_mutex_cond_lock(&session->cond_lock);
    /* get a free task and setup */
    task = list_first_entry_or_null(&session->list_done, MppDevTask, link_session);
    mpp_assert(task);

    task->req = ctx->reqs;
    task->req_cnt = ctx->req_cnt;

    list_del_init(&task->link_session);
    list_add_tail(&task->link_session, &session->list_wait);

    session->task_wait++;
    mpp_mutex_cond_unlock(&session->cond_lock);

    mpp_mutex_lock(&server->lock);
    task->task_id = server->task_id++;
    list_del_init(&task->link_server);
    list_add_tail(&task->link_server, &server->pending_task);
    server->pending_count++;
    mpp_serv_dbg_flow("session %d:%d add pending %d\n",
                      session->client, task->slot_idx, server->pending_count);

    mpp_timer_set_enable(server->timer, 1);
    mpp_mutex_unlock(&server->lock);

    return rk_ok;
}

rk_s32 wait_task(MppDevMppService *ctx, rk_s64 timeout)
{
    rk_s32 ret = rk_ok;
    MppDevTask *task = NULL;
    MppDevSession *session = (MppDevSession *)ctx->serv_ctx;
    (void) timeout;

    if (!session) {
        mpp_err_f("invalid ctx %p send task\n", ctx);
        return rk_nok;
    }

    task = list_first_entry_or_null(&session->list_wait, MppDevTask, link_session);
    mpp_assert(task);

    mpp_mutex_cond_lock(&session->cond_lock);
    if (session->task_wait != session->task_done) {
        mpp_serv_dbg_flow("session %d wait %d start %d:%d\n", session->client,
                          task->task_id, session->task_wait, session->task_done);
        mpp_mutex_cond_wait(&session->cond_lock);
    }
    mpp_serv_dbg_flow("session %d wait %d done %d:%d\n", session->client,
                      task->task_id, session->task_wait, session->task_done);
    mpp_mutex_cond_unlock(&session->cond_lock);

    list_del_init(&task->link_session);
    list_add_tail(&task->link_session, &session->list_done);

    return ret;
}

static MppDevBatServ *bat_server_get(MppDevServer *srv, MppClientType client_type)
{
    MppDevBatServ *server = NULL;
    char timer_name[32];

    mpp_mutex_lock(&srv->lock);

    server = srv->bat_server[client_type];
    if (server) {
        mpp_mutex_unlock(&srv->lock);
        return server;
    }

    server = mpp_calloc(MppDevBatServ, 1);
    if (!server) {
        mpp_err("mpp server failed to get bat server\n");
        mpp_mutex_unlock(&srv->lock);
        return NULL;
    }

    server->server_fd = open(srv->server_name, O_RDWR | O_CLOEXEC);
    if (server->server_fd < 0) {
        mpp_err("mpp server get bat server failed to open device\n");
        goto failed;
    }

    snprintf(timer_name, sizeof(timer_name) - 1, "%s_bat",
             strof_client_type(client_type));

    server->timer = mpp_timer_get(timer_name);
    if (!server->timer) {
        mpp_err("mpp server get bat server failed to create timer\n");
        goto failed;
    }

    mpp_mutex_init(&server->lock);

    mpp_timer_set_callback(server->timer, mpp_server_thread, server);
    /* 10ms */
    mpp_timer_set_timing(server->timer, 10, 10);

    INIT_LIST_HEAD(&server->session_list);
    INIT_LIST_HEAD(&server->list_batch);
    INIT_LIST_HEAD(&server->list_batch_free);
    INIT_LIST_HEAD(&server->pending_task);

    server->batch_pool = srv->batch_pool;
    server->max_task_in_batch = srv->max_task_in_batch;

    srv->bat_server[client_type] = server;
    mpp_mutex_unlock(&srv->lock);

    return server;

failed:
    if (server) {
        if (server->timer) {
            mpp_timer_put(server->timer);
            server->timer = NULL;
        }

        if (server->server_fd >= 0) {
            close(server->server_fd);
            server->server_fd = -1;
        }
        mpp_mutex_destroy(&server->lock);
    }
    MPP_FREE(server);
    mpp_mutex_unlock(&srv->lock);

    return NULL;
}

static rk_s32 bat_server_put(MppDevServer *srv, MppClientType client_type)
{
    MppDevBatServ *server = NULL;
    MppDevBatTask *batch, *n;

    mpp_mutex_lock(&srv->lock);

    if (!srv->bat_server[client_type]) {
        mpp_mutex_unlock(&srv->lock);
        return rk_ok;
    }

    server = srv->bat_server[client_type];
    srv->bat_server[client_type] = NULL;

    mpp_assert(server->batch_run == 0);
    mpp_assert(list_empty(&server->list_batch));
    mpp_assert(server->pending_count == 0);

    /* stop thread first */
    if (server->timer) {
        mpp_timer_put(server->timer);
        server->timer = NULL;
    }

    if (server->batch_free) {
        list_for_each_entry_safe(batch, n, &server->list_batch_free, MppDevBatTask, link_server) {
            batch_del(server, batch);
        }
    } else {
        mpp_assert(list_empty(&server->list_batch_free));
    }

    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }
    mpp_mutex_destroy(&server->lock);
    MPP_FREE(server);
    mpp_mutex_unlock(&srv->lock);

    return rk_ok;
}

static rk_s32 server_attach(MppDevServer *srv, MppDevMppService *ctx)
{
    MppClientType client_type = (MppClientType)ctx->client_type;
    MppDevBatServ *server;
    MppDevSession *session;
    rk_u32 i;

    if (client_type < 0 || client_type >= VPU_CLIENT_BUTT) {
        mpp_err("mpp server attach failed with invalid client type %d\n", client_type);
        return rk_nok;
    }

    /* if client type server is not created create it first */
    server = bat_server_get(srv, client_type);
    if (!server) {
        mpp_err("mpp server get bat server with client type %d failed\n", client_type);
        return rk_nok;
    }

    mpp_mutex_lock(&srv->lock);
    if (ctx->serv_ctx) {
        mpp_mutex_unlock(&srv->lock);
        return rk_ok;
    }

    session = (MppDevSession *)mpp_mem_pool_get_f(srv->session_pool);

    INIT_LIST_HEAD(&session->list_server);
    INIT_LIST_HEAD(&session->list_wait);
    INIT_LIST_HEAD(&session->list_done);

    session->ctx = ctx;
    session->server = server;
    session->client = ctx->client;
    mpp_mutex_cond_init(&session->cond_lock);
    session->task_wait = 0;
    session->task_done = 0;

    for (i = 0; i < MPP_ARRAY_ELEMS(session->tasks); i++) {
        MppDevTask *task = &session->tasks[i];

        INIT_LIST_HEAD(&task->link_server);
        INIT_LIST_HEAD(&task->link_session);
        INIT_LIST_HEAD(&task->link_batch);
        task->session = session;
        task->batch = NULL;
        task->task_id = -1;
        task->slot_idx = i;

        list_add_tail(&task->link_session, &session->list_done);
    }

    list_add_tail(&session->list_server, &server->session_list);
    ctx->serv_ctx = session;

    if (srv->enable) {
        ctx->batch_io = 1;
        ctx->server = server->server_fd;
    } else {
        ctx->batch_io = 0;
        ctx->server = ctx->client;
    }

    server->batch_max_count++;
    server->session_count++;
    mpp_mutex_unlock(&srv->lock);

    return rk_ok;
}

static rk_s32 server_detach(MppDevServer *srv, MppDevMppService *ctx)
{
    MppClientType client_type = (MppClientType)ctx->client_type;
    MppDevBatServ *server = bat_server_get(srv, client_type);
    MppDevSession *session = (MppDevSession *)ctx->serv_ctx;

    mpp_assert(server);

    mpp_mutex_lock(&srv->lock);
    if (!ctx->serv_ctx) {
        mpp_mutex_unlock(&srv->lock);
        return rk_ok;
    }

    ctx->server = ctx->client;
    ctx->serv_ctx = NULL;
    ctx->batch_io = 0;

    mpp_assert(server);
    mpp_assert(session);
    mpp_assert(session->client == ctx->client);
    mpp_assert(session->task_wait == session->task_done);
    mpp_assert(list_empty(&session->list_wait));

    list_del_init(&session->list_server);

    mpp_mutex_cond_destroy(&session->cond_lock);

    mpp_mem_pool_put_f(srv->session_pool, session);
    server->batch_max_count++;
    server->session_count++;

    mpp_mutex_unlock(&srv->lock);

    return rk_ok;
}

static void server_clear(MppDevServer *srv)
{
    if (srv) {
        if (srv->session_pool) {
            mpp_mem_pool_deinit_f(srv->session_pool);
            srv->session_pool = NULL;
        }

        if (srv->batch_pool) {
            mpp_mem_pool_deinit_f(srv->batch_pool);
            srv->batch_pool = NULL;
        }

        srv->inited = 0;
        srv->enable = 0;

        mpp_free(srv);
    }
}

static void mpp_server_init()
{
    MppDevServer *srv = srv_server;
    rk_s32 batch_task_size = 0;

    if (srv)
        return;

    srv = mpp_calloc(MppDevServer, 1);
    if (!srv) {
        mpp_err_f("failed to allocate mpp_server service\n");
        return;
    }

    srv_server = srv;

    srv->server_error = NULL;
    srv->server_name = NULL;
    srv->inited = 0;
    srv->enable = 1;
    srv->session_pool = NULL;
    srv->batch_pool = NULL;
    srv->max_task_in_batch = 0;
    srv->cmd_cap = NULL;

    mpp_env_get_u32("mpp_server_debug", &mpp_server_debug, 0);
    mpp_env_get_u32("mpp_server_enable", &srv->enable, 1);
    mpp_env_get_u32("mpp_server_batch_task", (rk_u32 *)&srv->max_task_in_batch,
                    MAX_BATCH_TASK);

    mpp_assert(srv->max_task_in_batch >= 1 && srv->max_task_in_batch <= 32);
    batch_task_size = sizeof(MppDevBatTask) + srv->max_task_in_batch *
                      (sizeof(MppReqV1) * (MAX_REQ_SEND_CNT + MAX_REQ_WAIT_CNT) +
                       sizeof(MppDevBatCmd));

    srv->cmd_cap = mpp_get_mpp_service_cmd_cap();
    if (rk_ok != mpp_service_check_cmd_valid(MPP_CMD_SET_SESSION_FD, srv->cmd_cap)) {
        srv->server_error = "mpp_service cmd not support";
        return;
    }

    mpp_mutex_init(&srv->lock);
    memset(srv->bat_server, 0, sizeof(srv->bat_server));

    do {
        srv->server_name = mpp_get_mpp_service_name();
        if (!srv->server_name) {
            srv->server_error = "get service device failed";
            break;
        }

        srv->session_pool = mpp_mem_pool_init_f("server_session", sizeof(MppDevSession));
        if (!srv->session_pool) {
            srv->server_error = "create session pool failed";
            break;
        }

        srv->batch_pool = mpp_mem_pool_init_f("server_batch", batch_task_size);
        if (!srv->batch_pool) {
            srv->server_error = "create batch tack pool failed";
            break;
        }

        srv->inited = 1;
    } while (0);

    if (!srv->inited) {
        server_clear(srv);
        srv_server = NULL;
    }
}

static void mpp_server_deinit()
{
    MppDevServer *srv = srv_server;

    srv_server = NULL;

    if (srv) {
        rk_s32 i;

        for (i = 0; i < VPU_CLIENT_BUTT; i++)
            bat_server_put(srv, (MppClientType)i);

        mpp_mutex_destroy(&srv->lock);
        server_clear(srv);
    }
}

rk_s32 mpp_server_attach(MppDev ctx)
{
    MppDevServer *srv = get_srv_server();
    rk_s32 ret = rk_nok;

    if (srv && ctx) {
        MppDevMppService *dev = (MppDevMppService *)ctx;

        ret = server_attach(srv, dev);
    }

    return ret;
}

rk_s32 mpp_server_detach(MppDev ctx)
{
    MppDevServer *srv = get_srv_server();
    rk_s32 ret = rk_nok;

    if (srv && ctx) {
        MppDevMppService *dev = (MppDevMppService *)ctx;

        ret = server_detach(srv, dev);
    }

    return ret;
}

rk_s32 mpp_server_send_task(MppDev ctx)
{
    MppDevServer *srv = get_srv_server();
    rk_s32 ret = rk_nok;

    if (srv && ctx)
        ret = send_task((MppDevMppService *)ctx);

    return ret;
}

rk_s32 mpp_server_wait_task(MppDev ctx, rk_s64 timeout)
{
    MppDevServer *srv = get_srv_server();
    rk_s32 ret = rk_nok;

    if (srv && ctx)
        ret = wait_task((MppDevMppService *)ctx, timeout);

    return ret;
}

MPP_SINGLETON(MPP_SGLN_SERVER, mpp_server, mpp_server_init, mpp_server_deinit)
