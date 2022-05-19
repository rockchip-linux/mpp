/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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
#include "mpp_mem_pool.h"

#include "mpp_device_debug.h"
#include "mpp_service_impl.h"
#include "mpp_server.h"

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

    RK_S32              slot_idx;

    /* lock by server */
    RK_S32              task_id;
    /* lock by batch */
    RK_S32              batch_slot_id;

    MppReqV1            *req;
    RK_S32              req_cnt;
};

struct MppDevBatTask_t {
    MppMutexCond        *cond;

    /* link to server */
    struct list_head    link_server;
    /* link to session tasks */
    struct list_head    link_tasks;

    RK_U32              batch_id;

    MppDevBatCmd        *bat_cmd;
    MppReqV1            *send_reqs;
    MppReqV1            *wait_reqs;

    /* lock and clean by server */
    RK_S32              send_req_cnt;
    RK_S32              wait_req_cnt;

    /* lock by server */
    RK_S32              fill_cnt;
    RK_S32              fill_full;
    RK_S32              fill_timeout;
    RK_S32              poll_cnt;
};

struct MppDevSession_t {
    MppMutexCond        *cond;

    /* hash table to server */
    struct list_head    list_server;
    /* link to session waiting tasks */
    struct list_head    list_wait;
    /* link to session free tasks */
    struct list_head    list_done;

    MppDevMppService    *ctx;
    MppDevBatServ       *server;

    RK_S32              client;

    RK_S32              task_wait;
    RK_S32              task_done;

    MppDevTask          tasks[MAX_SESSION_TASK];
};

struct MppDevBatServ_t {
    Mutex               *lock;

    RK_S32              server_fd;
    RK_U32              batch_id;
    RK_U32              task_id;

    /* timer for serializing process */
    MppTimer            timer;

    /* session register */
    struct list_head    session_list;
    RK_S32              session_count;

    /* batch task queue */
    struct list_head    list_batch;
    struct list_head    list_batch_free;
    MppMemPool          batch_pool;
    RK_S32              batch_max_count;
    RK_S32              batch_task_size;
    RK_S32              batch_run;
    RK_S32              batch_free;
    RK_S32              max_task_in_batch;

    /* link to all pending tasks */
    struct list_head    pending_task;
    RK_S32              pending_count;
};

RK_U32 mpp_server_debug = 0;

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
    MppDevBatTask *batch = (MppDevBatTask *)mpp_mem_pool_get(server->batch_pool);

    mpp_assert(batch);
    if (NULL == batch)
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

    mpp_mem_pool_put(server->batch_pool, batch);
    server->batch_free--;
    mpp_serv_dbg_flow("batch del free count %d:%d\n", server->batch_run, server->batch_free);
}

void batch_send(MppDevBatServ *server, MppDevBatTask *batch)
{
    RK_S32 ret = 0;

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
    Mutex *lock = server->lock;
    RK_S32 ret = MPP_OK;
    MppDevTask *task;
    MppDevBatTask *batch;
    MppDevSession *session = NULL;
    MppDevBatCmd *bat_cmd;
    MppReqV1 *req = NULL;
    RK_S32 pending = 0;

    mpp_serv_dbg_flow("process task start\n");

    /* 1. try poll and get finished task */
    do {
        batch = list_first_entry_or_null(&server->list_batch, MppDevBatTask, link_server);
        if (NULL == batch)
            break;

        mpp_assert(batch->wait_req_cnt);
        ret = mpp_service_ioctl_request(server->server_fd, batch->wait_reqs);
        if (!ret) {
            MppDevTask *n;

            list_for_each_entry_safe(task, n, &batch->link_tasks, MppDevTask, link_batch) {
                RK_S32 batch_slot_id = task->batch_slot_id;
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
                    session->cond->lock();
                    session->task_done++;
                    session->cond->signal();
                    session->cond->unlock();
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
    lock->lock();
    pending = server->pending_count;
    if (!pending && !server->batch_run && !server->session_count) {
        mpp_timer_set_enable(server->timer, 0);
        mpp_serv_dbg_flow("stop timer\n");
    }
    lock->unlock();

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
        if (NULL == batch) {
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
    lock->lock();
    task = list_first_entry_or_null(&server->pending_task, MppDevTask, link_server);
    list_del_init(&task->link_server);
    server->pending_count--;
    lock->unlock();
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
        RK_S32 i;

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

MPP_RET send_task(MppDevMppService *ctx)
{
    MppDevTask *task = NULL;
    MppDevSession *session = (MppDevSession *)ctx->serv_ctx;

    if (NULL == session || NULL == session->server) {
        mpp_err_f("invalid ctx %p session %p send task\n", ctx, session);
        return MPP_NOK;
    }

    MppDevBatServ *server = session->server;

    /* get free task from session and add to run list */
    session->cond->lock();
    /* get a free task and setup */
    task = list_first_entry_or_null(&session->list_done, MppDevTask, link_session);
    mpp_assert(task);

    task->req = ctx->reqs;
    task->req_cnt = ctx->req_cnt;

    list_del_init(&task->link_session);
    list_add_tail(&task->link_session, &session->list_wait);

    session->task_wait++;
    session->cond->unlock();

    server->lock->lock();
    task->task_id = server->task_id++;
    list_del_init(&task->link_server);
    list_add_tail(&task->link_server, &server->pending_task);
    server->pending_count++;
    mpp_serv_dbg_flow("session %d:%d add pending %d\n",
                      session->client, task->slot_idx, server->pending_count);

    mpp_timer_set_enable(server->timer, 1);
    server->lock->unlock();

    return MPP_OK;
}

MPP_RET wait_task(MppDevMppService *ctx, RK_S64 timeout)
{
    RK_S32 ret = MPP_OK;
    MppDevTask *task = NULL;
    MppDevSession *session = (MppDevSession *)ctx->serv_ctx;
    (void) timeout;

    if (NULL == session) {
        mpp_err_f("invalid ctx %p send task\n", ctx);
        return MPP_NOK;
    }

    task = list_first_entry_or_null(&session->list_wait, MppDevTask, link_session);
    mpp_assert(task);

    session->cond->lock();
    if (session->task_wait != session->task_done) {
        mpp_serv_dbg_flow("session %d wait %d start %d:%d\n", session->client,
                          task->task_id, session->task_wait, session->task_done);
        session->cond->wait();
    }
    mpp_serv_dbg_flow("session %d wait %d done %d:%d\n", session->client,
                      task->task_id, session->task_wait, session->task_done);
    session->cond->unlock();

    list_del_init(&task->link_session);
    list_add_tail(&task->link_session, &session->list_done);

    mpp_assert(session->task_wait == session->task_done);

    return (MPP_RET)ret;
}

class MppDevServer : Mutex
{
private:
    // avoid any unwanted function
    MppDevServer();
    ~MppDevServer();
    MppDevServer(const MppDevServer &);
    MppDevServer &operator=(const MppDevServer &);

    void clear();

    const char          *mServerError;
    const char          *mServerName;
    RK_S32              mInited;
    RK_U32              mEnable;

    MppDevBatServ       *mBatServer[VPU_CLIENT_BUTT];

    MppMemPool          mSessionPool;
    MppMemPool          mBatchPool;

    RK_S32              mMaxTaskInBatch;

    const MppServiceCmdCap *mCmdCap;

public:
    static MppDevServer *get_inst() {
        static MppDevServer inst;
        return &inst;
    }

    MppDevBatServ  *bat_server_get(MppClientType client_type);
    MPP_RET         bat_server_put(MppClientType client_type);

    MPP_RET attach(MppDevMppService *ctx);
    MPP_RET detach(MppDevMppService *ctx);

    MPP_RET check_status(void);
};

MppDevServer::MppDevServer() :
    mServerError(NULL),
    mServerName(NULL),
    mInited(0),
    mEnable(1),
    mSessionPool(NULL),
    mBatchPool(NULL),
    mMaxTaskInBatch(0),
    mCmdCap(NULL)
{
    RK_S32 batch_task_size = 0;

    mpp_env_get_u32("mpp_server_debug", &mpp_server_debug, 0);
    mpp_env_get_u32("mpp_server_enable", &mEnable, 1);
    mpp_env_get_u32("mpp_server_batch_task", (RK_U32 *)&mMaxTaskInBatch,
                    MAX_BATCH_TASK);

    mpp_assert(mMaxTaskInBatch >= 1 && mMaxTaskInBatch <= 32);
    batch_task_size = sizeof(MppDevBatTask) + mMaxTaskInBatch *
                      (sizeof(MppReqV1) * (MAX_REQ_SEND_CNT + MAX_REQ_WAIT_CNT) +
                       sizeof(MppDevBatCmd));

    mCmdCap = mpp_get_mpp_service_cmd_cap();
    if (MPP_OK != mpp_service_check_cmd_valid(MPP_CMD_SET_SESSION_FD, mCmdCap)) {
        mServerError = "mpp_service cmd not support";
        return;
    }

    do {
        mServerName = mpp_get_mpp_service_name();
        if (NULL == mServerName) {
            mServerError = "get service device failed";
            break;
        }

        mSessionPool = mpp_mem_pool_init(sizeof(MppDevSession));
        if (NULL == mSessionPool) {
            mServerError = "create session pool failed";
            break;
        }

        mBatchPool = mpp_mem_pool_init(batch_task_size);
        if (NULL == mBatchPool) {
            mServerError = "create batch tack pool failed";
            break;
        }

        mInited = 1;
    } while (0);

    if (!mInited) {
        clear();
        return;
    }

    memset(mBatServer, 0, sizeof(mBatServer));
}

MppDevServer::~MppDevServer()
{
    RK_S32 i;

    for (i = 0; i < VPU_CLIENT_BUTT; i++)
        bat_server_put((MppClientType)i);

    clear();
}

void MppDevServer::clear()
{
    if (mSessionPool) {
        mpp_mem_pool_deinit(mSessionPool);
        mSessionPool = NULL;
    }

    if (mBatchPool) {
        mpp_mem_pool_deinit(mBatchPool);
        mBatchPool = NULL;
    }
    mInited = 0;
    mEnable = 0;
}

MppDevBatServ *MppDevServer::bat_server_get(MppClientType client_type)
{
    MppDevBatServ *server = NULL;

    AutoMutex auto_lock(this);

    server = mBatServer[client_type];
    if (server)
        return server;

    server = mpp_calloc(MppDevBatServ, 1);
    if (NULL == server) {
        mpp_err("mpp server failed to get bat server\n");
        return NULL;
    }

    server->server_fd = open(mServerName, O_RDWR | O_CLOEXEC);
    if (server->server_fd < 0) {
        mpp_err("mpp server get bat server failed to open device\n");
        goto failed;
    }

    char timer_name[32];

    snprintf(timer_name, sizeof(timer_name) - 1, "%s_bat",
             strof_client_type(client_type));

    server->timer = mpp_timer_get(timer_name);
    if (NULL == server->timer) {
        mpp_err("mpp server get bat server failed to create timer\n");
        goto failed;
    }

    server->lock = new Mutex();
    if (NULL == server->lock) {
        mpp_err("mpp server get bat server failed to create mutex\n");
        goto failed;
    }

    mpp_timer_set_callback(server->timer, mpp_server_thread, server);
    /* 10ms */
    mpp_timer_set_timing(server->timer, 10, 10);

    INIT_LIST_HEAD(&server->session_list);
    INIT_LIST_HEAD(&server->list_batch);
    INIT_LIST_HEAD(&server->list_batch_free);
    INIT_LIST_HEAD(&server->pending_task);

    server->batch_pool = mBatchPool;
    server->max_task_in_batch = mMaxTaskInBatch;

    mBatServer[client_type] = server;
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
        if (server->lock) {
            delete server->lock;
            server->lock = NULL;
        }
    }
    MPP_FREE(server);
    return server;
}

MPP_RET MppDevServer::bat_server_put(MppClientType client_type)
{
    MppDevBatTask *batch, *n;
    MppDevBatServ *server = NULL;
    AutoMutex auto_lock(this);

    if (NULL == mBatServer[client_type])
        return MPP_OK;

    server = mBatServer[client_type];
    mBatServer[client_type] = NULL;

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
    if (server->lock) {
        delete server->lock;
        server->lock = NULL;
    }
    MPP_FREE(server);
    return MPP_OK;
}

MPP_RET MppDevServer::attach(MppDevMppService *ctx)
{
    RK_U32 i;

    if (!mInited) {
        mpp_err("mpp server failed for %s\n", mServerError);
        return MPP_NOK;
    }

    MppClientType client_type = (MppClientType)ctx->client_type;

    if (client_type < 0 || client_type >= VPU_CLIENT_BUTT) {
        mpp_err("mpp server attach failed with invalid client type %d\n", client_type);
        return MPP_NOK;
    }

    /* if client type server is not created create it first */
    MppDevBatServ *server = bat_server_get(client_type);
    if (NULL == server) {
        mpp_err("mpp server get bat server with client type %d failed\n", client_type);
        return MPP_NOK;
    }

    AutoMutex auto_lock(server->lock);
    if (ctx->serv_ctx)
        return MPP_OK;

    MppDevSession *session = (MppDevSession *)mpp_mem_pool_get(mSessionPool);
    INIT_LIST_HEAD(&session->list_server);
    INIT_LIST_HEAD(&session->list_wait);
    INIT_LIST_HEAD(&session->list_done);

    session->ctx = ctx;
    session->server = server;
    session->client = ctx->client;
    session->cond = new MppMutexCond();
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

    if (mEnable) {
        ctx->batch_io = 1;
        ctx->server = server->server_fd;
    } else {
        ctx->batch_io = 0;
        ctx->server = ctx->client;
    }

    server->batch_max_count++;
    server->session_count++;

    return MPP_OK;
}

MPP_RET MppDevServer::detach(MppDevMppService *ctx)
{
    if (!mInited) {
        mpp_err("mpp server failed for %s\n", mServerError);
        return MPP_NOK;
    }

    MppClientType client_type = (MppClientType)ctx->client_type;

    MppDevBatServ *server = bat_server_get(client_type);
    MppDevSession *session = (MppDevSession *)ctx->serv_ctx;

    mpp_assert(server);

    AutoMutex auto_lock(server->lock);
    if (NULL == ctx->serv_ctx)
        return MPP_OK;

    ctx->server = ctx->client;
    ctx->serv_ctx = NULL;
    ctx->batch_io = 0;

    mpp_assert(server);
    mpp_assert(session);
    mpp_assert(session->client == ctx->client);
    mpp_assert(session->task_wait == session->task_done);
    mpp_assert(list_empty(&session->list_wait));

    list_del_init(&session->list_server);

    if (session->cond) {
        delete session->cond;
        session->cond = NULL;
    }

    mpp_mem_pool_put(mSessionPool, session);
    server->batch_max_count++;
    server->session_count++;

    return MPP_OK;
}

MPP_RET MppDevServer::check_status(void)
{
    if (!mInited) {
        mpp_err("mpp server failed for %s\n", mServerError);
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpp_server_attach(MppDev ctx)
{
    MppDevMppService *dev = (MppDevMppService *)ctx;

    return MppDevServer::get_inst()->attach(dev);
}

MPP_RET mpp_server_detach(MppDev ctx)
{
    MppDevMppService *dev = (MppDevMppService *)ctx;

    return MppDevServer::get_inst()->detach(dev);
}

MPP_RET mpp_server_send_task(MppDev ctx)
{
    MPP_RET ret = MppDevServer::get_inst()->check_status();
    if (!ret)
        ret = send_task((MppDevMppService *)ctx);

    return ret;
}

MPP_RET mpp_server_wait_task(MppDev ctx, RK_S64 timeout)
{
    MPP_RET ret = MppDevServer::get_inst()->check_status();
    if (!ret)
        ret = wait_task((MppDevMppService *)ctx, timeout);

    return ret;
}
