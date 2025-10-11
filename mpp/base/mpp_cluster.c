/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp_cluster"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_lock.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "mpp_cluster.h"
#include "mpp_dev_defs.h"

#define MPP_CLUSTER_DBG_FLOW            (0x00000001)
#define MPP_CLUSTER_DBG_LOCK            (0x00000002)

#define cluster_dbg(flag, fmt, ...)     _mpp_dbg(mpp_cluster_debug, flag, fmt, ## __VA_ARGS__)
#define cluster_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpp_cluster_debug, flag, fmt, ## __VA_ARGS__)

#define cluster_dbg_flow(fmt, ...)      cluster_dbg(MPP_CLUSTER_DBG_FLOW, fmt, ## __VA_ARGS__)
#define cluster_dbg_lock(fmt, ...)      cluster_dbg(MPP_CLUSTER_DBG_LOCK, fmt, ## __VA_ARGS__)

RK_U32 mpp_cluster_debug = 0;
RK_U32 mpp_cluster_thd_cnt = 1;

typedef struct MppNodeProc_s    MppNodeProc;
typedef struct MppNodeTask_s    MppNodeTask;
typedef struct MppNodeImpl_s    MppNodeImpl;

typedef struct ClusterQueue_s   ClusterQueue;
typedef struct ClusterWorker_s  ClusterWorker;
typedef struct MppCluster_s     MppCluster;

#define NODE_VALID              (0x00000001)
#define NODE_IDLE               (0x00000002)
#define NODE_SIGNAL             (0x00000004)
#define NODE_WAIT               (0x00000008)
#define NODE_RUN                (0x00000010)

#define NODE_ACT_NONE           (0x00000000)
#define NODE_ACT_IDLE_TO_WAIT   (0x00000001)
#define NODE_ACT_RUN_TO_SIGNAL  (0x00000002)

typedef enum MppWorkerState_e {
    WORKER_IDLE,
    WORKER_RUNNING,

    WORKER_STATE_BUTT,
} MppWorkerState;

struct MppNodeProc_s {
    TaskProc                proc;
    void                    *param;

    /* timing statistic */
    RK_U32                  run_count;
    RK_S64                  run_time;
};

struct MppNodeTask_s {
    struct list_head        list_sched;
    MppNodeImpl             *node;
    const char              *node_name;

    /* lock ptr to cluster queue lock */
    ClusterQueue            *queue;

    MppNodeProc             *proc;
};

/* MppNode will be embeded in MppCtx */
struct MppNodeImpl_s {
    char                    name[32];
    /* list linked to scheduler */
    RK_S32                  node_id;
    RK_U32                  state;

    MppNodeProc             work;

    RK_U32                  priority;
    RK_S32                  attached;
    sem_t                   sem_detach;

    /* for cluster schedule */
    MppNodeTask             task;
};

struct ClusterQueue_s {
    MppCluster              *cluster;

    pthread_mutex_t         lock;
    struct list_head        list;
    RK_S32                  count;
};

struct ClusterWorker_s {
    char                    name[32];
    MppCluster              *cluster;
    RK_S32                  worker_id;

    MppThread               *thd;
    MppWorkerState          state;

    RK_S32                  batch_count;
    RK_S32                  work_count;
    struct list_head        list_task;
};

struct MppCluster_s {
    char                    name[16];
    pid_t                   pid;
    RK_S32                  client_type;
    RK_S32                  node_id;
    RK_S32                  worker_id;

    ClusterQueue            queue[MAX_PRIORITY];
    RK_S32                  node_count;

    /* multi-worker info */
    RK_S32                  worker_count;
    ClusterWorker           *worker;
    MppThreadFunc           worker_func;
};

typedef struct MppClusterServer_s {
    MppMutex                mutex;
    MppCluster              *clusters[VPU_CLIENT_BUTT];
} MppClusterServer;

static MppClusterServer *srv_cluster = NULL;

static MppCluster *cluster_server_get(MppClientType client_type);
static MPP_RET cluster_server_put(MppClientType client_type);

#define mpp_node_task_schedule(task) \
    mpp_node_task_schedule_f(__FUNCTION__, task)

#define mpp_node_task_schedule_from(caller, task) \
    mpp_node_task_schedule_f(caller, task)

#define cluster_queue_lock(queue)   cluster_queue_lock_f(__FUNCTION__, queue)
#define cluster_queue_unlock(queue) cluster_queue_unlock_f(__FUNCTION__, queue)

#define get_srv_cluster_f() \
    ({ \
        MppClusterServer *__tmp; \
        if (srv_cluster) { \
            __tmp = srv_cluster; \
        } else { \
            mpp_cluster_srv_init(); \
            __tmp = srv_cluster; \
            if (!__tmp) \
                mpp_err("mpp cluster srv not init at %s\n", __FUNCTION__); \
        } \
        __tmp; \
    })

static MPP_RET cluster_queue_lock_f(const char *caller, ClusterQueue *queue)
{
    MppCluster *cluster = queue->cluster;
    RK_S32 ret;

    cluster_dbg_lock("%s lock queue at %s start\n", cluster->name, caller);

    ret = pthread_mutex_lock(&queue->lock);

    cluster_dbg_lock("%s lock queue at %s ret %d \n", cluster->name, caller, ret);

    return (ret) ? MPP_NOK : MPP_OK;
}

static MPP_RET cluster_queue_unlock_f(const char *caller, ClusterQueue *queue)
{
    MppCluster *cluster = queue->cluster;
    RK_S32 ret;

    cluster_dbg_lock("%s unlock queue at %s start\n", cluster->name, caller);

    ret = pthread_mutex_unlock(&queue->lock);

    cluster_dbg_lock("%s unlock queue at %s ret %d \n", cluster->name, caller, ret);

    return (ret) ? MPP_NOK : MPP_OK;
}

void cluster_signal_f(const char *caller, MppCluster *p);

MPP_RET mpp_cluster_queue_init(ClusterQueue *queue, MppCluster *cluster)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&queue->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    queue->cluster = cluster;
    INIT_LIST_HEAD(&queue->list);
    queue->count = 0;

    return MPP_OK;
}

MPP_RET mpp_cluster_queue_deinit(ClusterQueue *queue)
{
    mpp_assert(!queue->count);
    mpp_assert(list_empty(&queue->list));

    pthread_mutex_destroy(&queue->lock);

    return MPP_OK;
}

MPP_RET mpp_node_task_attach(MppNodeTask *task, MppNodeImpl *node,
                             ClusterQueue *queue, MppNodeProc *proc)
{
    INIT_LIST_HEAD(&task->list_sched);

    task->node = node;
    task->node_name = node->name;

    task->queue = queue;
    task->proc = proc;

    node->state = NODE_VALID | NODE_IDLE;
    node->attached = 1;

    return MPP_OK;
}

MPP_RET mpp_node_task_schedule_f(const char *caller, MppNodeTask *task)
{
    ClusterQueue *queue = task->queue;
    MppCluster *cluster = queue->cluster;
    MppNodeImpl *node = task->node;
    MppNodeProc *proc = task->proc;
    const char *node_name = task->node_name;
    RK_U32 new_st;
    RK_U32 action = NODE_ACT_NONE;
    bool ret = false;

    cluster_dbg_flow("%s sched from %s before [%d:%d] queue %d\n",
                     node_name, caller, node->state, proc->run_count, queue->count);

    do {
        RK_U32 old_st = node->state;

        action = NODE_ACT_NONE;
        new_st = 0;

        if (old_st & NODE_WAIT) {
            cluster_dbg_flow("%s sched task %x stay  wait\n", node_name, old_st);
            break;
        }

        if (old_st & NODE_IDLE) {
            new_st = old_st ^ (NODE_IDLE | NODE_WAIT);
            cluster_dbg_flow("%s sched task %x -> %x wait\n", node_name, old_st, new_st);
            action = NODE_ACT_IDLE_TO_WAIT;
        } else if (old_st & NODE_RUN) {
            new_st = old_st | NODE_SIGNAL;
            action = NODE_ACT_RUN_TO_SIGNAL;
            cluster_dbg_flow("%s sched task %x -> %x signal\n", node_name, old_st, new_st);
        } else {
            cluster_dbg_flow("%s sched task %x unknow state %x\n", node_name, old_st);
        }

        ret = MPP_BOOL_CAS(&node->state, old_st, new_st);
        cluster_dbg_flow("%s sched task %x -> %x cas ret %d act %d\n",
                         node_name, old_st, new_st, ret, action);
    } while (!ret);

    switch (action) {
    case NODE_ACT_IDLE_TO_WAIT : {
        cluster_queue_lock(queue);
        mpp_assert(list_empty(&task->list_sched));
        list_add_tail(&task->list_sched, &queue->list);
        queue->count++;
        cluster_dbg_flow("%s sched task -> wq %s:%d\n", node_name, cluster->name, queue->count);
        cluster_queue_unlock(queue);

        cluster_dbg_flow("%s sched signal from %s\n", node_name, caller);
        cluster_signal_f(caller, cluster);
    } break;
    case NODE_ACT_RUN_TO_SIGNAL : {
        cluster_dbg_flow("%s sched signal from %s\n", node_name, caller);
        cluster_signal_f(caller, cluster);
    } break;
    }

    cluster_dbg_flow("%s sched from %s after  [%d:%d] queue %d\n",
                     node_name, caller, node->state, proc->run_count, queue->count);

    return MPP_OK;
}

MPP_RET mpp_node_task_detach(MppNodeTask *task)
{
    MppNodeImpl *node = task->node;
    MPP_RET ret = MPP_OK;

    if (node->attached) {
        const char *node_name = task->node_name;
        MppNodeProc *proc = task->proc;

        MPP_FETCH_AND(&node->state, ~NODE_VALID);

        mpp_node_task_schedule(task);

        cluster_dbg_flow("%s state %x:%d wait detach start\n",
                         node_name, node->state, proc->run_count);

        sem_wait(&node->sem_detach);
        mpp_assert(node->attached == 0);

        cluster_dbg_flow("%s state %x:%d wait detach done\n",
                         node_name, node->state, proc->run_count);
    }

    return ret;
}

MPP_RET mpp_node_init(MppNode *node)
{
    MppNodeImpl *p = mpp_calloc(MppNodeImpl, 1);
    if (p)
        sem_init(&p->sem_detach, 0, 0);

    *node = p;

    return p ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_node_deinit(MppNode node)
{
    MppNodeImpl *p = (MppNodeImpl *)node;

    if (p) {
        if (p->attached)
            mpp_node_task_detach(&p->task);

        mpp_assert(p->attached == 0);

        sem_destroy(&p->sem_detach);

        mpp_free(p);
    }

    return MPP_OK;
}

MPP_RET mpp_node_set_func(MppNode node, TaskProc proc, void *param)
{
    MppNodeImpl *p = (MppNodeImpl *)node;
    if (!p)
        return MPP_NOK;

    p->work.proc = proc;
    p->work.param = param;

    return MPP_OK;
}

MPP_RET cluster_worker_init(ClusterWorker *p, MppCluster *cluster)
{
    MppThread *thd = NULL;
    MPP_RET ret = MPP_NOK;

    INIT_LIST_HEAD(&p->list_task);
    p->worker_id = cluster->worker_id++;

    p->batch_count = 1;
    p->work_count = 0;
    p->cluster = cluster;
    p->state = WORKER_IDLE;
    snprintf(p->name, sizeof(p->name) - 1, "%d:W%d", cluster->pid, p->worker_id);
    thd = mpp_thread_create(cluster->worker_func, p, p->name);
    if (thd) {
        p->thd = thd;
        mpp_thread_start(thd);
        ret = MPP_OK;
    }

    return ret;
}

MPP_RET cluster_worker_deinit(ClusterWorker *p)
{
    if (p->thd) {
        mpp_thread_stop(p->thd);
        mpp_thread_destroy(p->thd);
        p->thd = NULL;
    }

    mpp_assert(list_empty(&p->list_task));
    mpp_assert(p->work_count == 0);

    p->batch_count = 0;
    p->cluster = NULL;

    return MPP_OK;
}

RK_S32 cluster_worker_get_task(ClusterWorker *p)
{
    MppCluster *cluster = p->cluster;
    RK_S32 batch_count = p->batch_count;
    RK_S32 count = 0;
    RK_U32 new_st;
    RK_U32 old_st;
    bool ret;
    RK_S32 i;

    cluster_dbg_flow("%s get %d task start\n", p->name, batch_count);

    for (i = 0; i < MAX_PRIORITY; i++) {
        ClusterQueue *queue = &cluster->queue[i];
        MppNodeTask *task = NULL;
        MppNodeImpl *node = NULL;

        do {
            cluster_queue_lock(queue);

            if (list_empty(&queue->list)) {
                mpp_assert(queue->count == 0);
                cluster_dbg_flow("%s get P%d task ret no task\n", p->name, i);
                cluster_queue_unlock(queue);
                break;
            }

            mpp_assert(queue->count);
            task = list_first_entry(&queue->list, MppNodeTask, list_sched);
            list_del_init(&task->list_sched);
            node = task->node;

            queue->count--;

            do {
                old_st = node->state;
                new_st = old_st ^ (NODE_WAIT | NODE_RUN);

                mpp_assert(old_st & NODE_WAIT);
                ret = MPP_BOOL_CAS(&node->state, old_st, new_st);
            } while (!ret);

            list_add_tail(&task->list_sched, &p->list_task);
            p->work_count++;
            count++;

            cluster_dbg_flow("%s get P%d %s -> rq %d\n", p->name, i, node->name, p->work_count);

            cluster_queue_unlock(queue);

            if (count >= batch_count)
                break;
        } while (1);

        if (count >= batch_count)
            break;
    }

    cluster_dbg_flow("%s get %d task ret %d\n", p->name, batch_count, count);

    return count;
}

static void cluster_worker_run_task(ClusterWorker *p)
{
    RK_U32 new_st;
    RK_U32 old_st;
    bool cas_ret;

    cluster_dbg_flow("%s run %d work start\n", p->name, p->work_count);

    while (!list_empty(&p->list_task)) {
        MppNodeTask *task = list_first_entry(&p->list_task, MppNodeTask, list_sched);
        MppNodeProc *proc = task->proc;
        MppNodeImpl *node = task->node;
        RK_S64 time_start;
        RK_S64 time_end;
        RK_U32 state;
        MPP_RET proc_ret;

        /* check trigger for re-add task */
        cluster_dbg_flow("%s run %s start atate %d\n", p->name, task->node_name, node->state);
        mpp_assert(node->state & NODE_RUN);
        if (!(node->state & NODE_RUN))
            mpp_err_f("%s run state check %x is invalid on run", p->name, node->state);

        time_start = mpp_time();
        proc_ret = proc->proc(proc->param);
        time_end = mpp_time();

        cluster_dbg_flow("%s run %s ret %d\n", p->name, task->node_name, proc_ret);
        proc->run_time += time_end - time_start;
        proc->run_count++;

        state = node->state;
        if (!(state & NODE_VALID)) {
            cluster_dbg_flow("%s run found destroy\n", p->name);
            list_del_init(&task->list_sched);
            node->attached = 0;

            sem_post(&node->sem_detach);
            cluster_dbg_flow("%s run sem post done\n", p->name);
        } else if (state & NODE_SIGNAL) {
            ClusterQueue *queue = task->queue;

            list_del_init(&task->list_sched);

            do {
                old_st = state;
                // NOTE: clear NODE_RUN and NODE_SIGNAL, set NODE_WAIT
                new_st = old_st ^ (NODE_SIGNAL | NODE_WAIT | NODE_RUN);
                cas_ret = MPP_BOOL_CAS(&node->state, old_st, new_st);
            } while (!cas_ret);

            cluster_dbg_flow("%s run state %x -> %x signal -> wait\n", p->name, old_st, new_st);

            cluster_queue_lock(queue);
            list_add_tail(&task->list_sched, &queue->list);
            queue->count++;
            cluster_queue_unlock(queue);
        } else {
            list_del_init(&task->list_sched);
            do {
                old_st = node->state;
                new_st = old_st ^ (NODE_IDLE | NODE_RUN);

                cas_ret = MPP_BOOL_CAS(&node->state, old_st, new_st);
            } while (!cas_ret);
            mpp_assert(node->state & NODE_IDLE);
            mpp_assert(!(node->state & NODE_RUN));

            cluster_dbg_flow("%s run state %x -> %x run -> idle\n", p->name, old_st, new_st);
        }

        p->work_count--;
    }

    mpp_assert(p->work_count == 0);

    cluster_dbg_flow("%s run all done\n", p->name);
}

static void *cluster_worker(void *data)
{
    ClusterWorker *p = (ClusterWorker *)data;
    MppThread *thd = p->thd;

    while (1) {
        {
            RK_S32 task_count = 0;

            cluster_dbg_lock("%s lock start\n", p->name);
            mpp_thread_lock(thd, THREAD_WORK);
            cluster_dbg_lock("%s lock done\n", p->name);

            if (MPP_THREAD_RUNNING != mpp_thread_get_status(thd, THREAD_WORK)) {
                mpp_thread_unlock(thd, THREAD_WORK);
                break;
            }

            task_count = cluster_worker_get_task(p);
            if (!task_count) {
                p->state = WORKER_IDLE;
                mpp_thread_wait(thd, THREAD_WORK);
                p->state = WORKER_RUNNING;
            }
            mpp_thread_unlock(thd, THREAD_WORK);
        }

        cluster_worker_run_task(p);
    }

    return NULL;
}

void cluster_signal_f(const char *caller, MppCluster *p)
{
    RK_S32 i;

    cluster_dbg_flow("%s signal from %s\n", p->name, caller);

    for (i = 0; i < p->worker_count; i++) {
        ClusterWorker *worker = &p->worker[i];
        MppThread *thd = worker->thd;

        mpp_thread_lock(thd, THREAD_WORK);

        if (worker->state == WORKER_IDLE) {
            mpp_thread_signal(thd, THREAD_WORK);
            cluster_dbg_flow("%s signal\n", p->name);
            mpp_thread_unlock(thd, THREAD_WORK);
            break;
        }

        mpp_thread_unlock(thd, THREAD_WORK);
    }
}

static void mpp_cluster_srv_init(void)
{
    MppClusterServer *srv = srv_cluster;

    mpp_env_get_u32("mpp_cluster_debug", &mpp_cluster_debug, 0);
    mpp_env_get_u32("mpp_cluster_thd_cnt", &mpp_cluster_thd_cnt, 1);

    if (srv)
        return;

    srv = mpp_calloc(MppClusterServer, 1);
    if (!srv) {
        mpp_err_f("failed to allocate cluster server\n");
        return;
    }

    memset(srv->clusters, 0, sizeof(srv->clusters));
    mpp_mutex_init(&srv->mutex);

    srv_cluster = srv;
}

static void mpp_cluster_srv_deinit(void)
{
    MppClusterServer *srv = srv_cluster;
    RK_S32 i;

    if (!srv)
        return;

    for (i = 0; i < VPU_CLIENT_BUTT; i++)
        cluster_server_put((MppClientType)i);

    mpp_mutex_destroy(&srv->mutex);
    MPP_FREE(srv_cluster);
}

MPP_SINGLETON(MPP_SGLN_SYS_CFG, mpp_cluster, mpp_cluster_srv_init, mpp_cluster_srv_deinit)

static MppCluster *cluster_server_get(MppClientType client_type)
{
    MppClusterServer *srv = get_srv_cluster_f();
    RK_S32 i;
    MppCluster *p = NULL;

    if (!srv || client_type >= VPU_CLIENT_BUTT)
        goto done;

    {
        mpp_mutex_lock(&srv->mutex);

        p = srv->clusters[client_type];
        if (p) {
            mpp_mutex_unlock(&srv->mutex);
            goto done;
        }

        p = mpp_malloc(MppCluster, 1);
        if (p) {
            for (i = 0; i < MAX_PRIORITY; i++)
                mpp_cluster_queue_init(&p->queue[i], p);

            p->pid  = getpid();
            p->client_type = client_type;
            snprintf(p->name, sizeof(p->name) - 1, "%d:%d", p->pid, client_type);
            p->node_id = 0;
            p->worker_id = 0;
            p->worker_func = cluster_worker;
            p->worker_count = mpp_cluster_thd_cnt;

            mpp_assert(p->worker_count > 0);

            p->worker = mpp_malloc(ClusterWorker, p->worker_count);

            for (i = 0; i < p->worker_count; i++)
                cluster_worker_init(&p->worker[i], p);

            srv->clusters[client_type] = p;
            cluster_dbg_flow("%s created\n", p->name);
        }

        mpp_mutex_unlock(&srv->mutex);
    }

done:
    if (p)
        cluster_dbg_flow("%s get\n", p->name);
    else
        cluster_dbg_flow("%d get cluster %d failed\n", getpid(), client_type);

    return p;
}

static MPP_RET cluster_server_put(MppClientType client_type)
{
    MppClusterServer *srv = get_srv_cluster_f();
    MppCluster *p;
    RK_S32 i;

    if (!srv || client_type >= VPU_CLIENT_BUTT)
        return MPP_NOK;

    mpp_mutex_lock(&srv->mutex);

    p = srv->clusters[client_type];
    if (!p) {
        mpp_mutex_unlock(&srv->mutex);
        return MPP_NOK;
    }

    for (i = 0; i < p->worker_count; i++)
        cluster_worker_deinit(&p->worker[i]);

    for (i = 0; i < MAX_PRIORITY; i++)
        mpp_cluster_queue_deinit(&p->queue[i]);

    cluster_dbg_flow("put %s\n", p->name);

    MPP_FREE(p->worker);
    mpp_free(p);
    srv->clusters[client_type] = NULL;

    mpp_mutex_unlock(&srv->mutex);

    return MPP_OK;
}

MPP_RET mpp_node_attach(MppNode node, MppClientType type)
{
    MppNodeImpl *impl = (MppNodeImpl *)node;
    MppCluster *p = cluster_server_get(type);
    RK_U32 priority = impl->priority;
    ClusterQueue *queue = &p->queue[priority];

    mpp_assert(priority < MAX_PRIORITY);
    mpp_assert(p);

    impl->node_id = MPP_FETCH_ADD(&p->node_id, 1);

    snprintf(impl->name, sizeof(impl->name) - 1, "%s:%d", p->name, impl->node_id);

    mpp_node_task_attach(&impl->task, impl, queue, &impl->work);

    MPP_FETCH_ADD(&p->node_count, 1);

    cluster_dbg_flow("%s:%d attached %d\n", p->name, impl->node_id, p->node_count);

    /* attach and run once first */
    mpp_node_task_schedule(&impl->task);
    cluster_dbg_flow("%s trigger signal from %s\n", impl->name, __FUNCTION__);

    return MPP_OK;
}

MPP_RET mpp_node_detach(MppNode node)
{
    MppNodeImpl *impl = (MppNodeImpl *)node;

    mpp_node_task_detach(&impl->task);
    cluster_dbg_flow("%s detached\n", impl->name);

    return MPP_OK;
}

MPP_RET mpp_node_trigger_f(const char *caller, MppNode node, RK_S32 trigger)
{
    if (trigger) {
        MppNodeImpl *impl = (MppNodeImpl *)node;

        mpp_node_task_schedule_from(caller, &impl->task);
    }

    return MPP_OK;
}
