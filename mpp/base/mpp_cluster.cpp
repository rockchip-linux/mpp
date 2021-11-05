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

#define  MODULE_TAG "mpp_cluster"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_lock.h"
#include "mpp_time.h"

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

#define mpp_node_task_schedule(task) \
    mpp_node_task_schedule_f(__FUNCTION__, task)

#define mpp_node_task_schedule_from(caller, task) \
    mpp_node_task_schedule_f(caller, task)

#define cluster_queue_lock(queue)   cluster_queue_lock_f(__FUNCTION__, queue)
#define cluster_queue_unlock(queue) cluster_queue_unlock_f(__FUNCTION__, queue)

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
    thd = new MppThread(cluster->worker_func, p, p->name);
    if (thd) {
        p->thd = thd;
        thd->start();
        ret = MPP_OK;
    }

    return ret;
}

MPP_RET cluster_worker_deinit(ClusterWorker *p)
{
    if (p->thd) {
        p->thd->stop();
        delete p->thd;
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
            AutoMutex autolock(thd->mutex());
            cluster_dbg_lock("%s lock done\n", p->name);

            if (MPP_THREAD_RUNNING != thd->get_status())
                break;

            task_count = cluster_worker_get_task(p);
            if (!task_count) {
                p->state = WORKER_IDLE;
                thd->wait();
                p->state = WORKER_RUNNING;
            }
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
        AutoMutex auto_lock(thd->mutex());

        if (worker->state == WORKER_IDLE) {
            thd->signal();
            cluster_dbg_flow("%s signal\n", p->name);
            break;
        }
    }
}

class MppClusterServer;

MppClusterServer *cluster_server = NULL;

class MppClusterServer : Mutex
{
private:
    // avoid any unwanted function
    MppClusterServer();
    ~MppClusterServer();
    MppClusterServer(const MppClusterServer &);
    MppClusterServer &operator=(const MppClusterServer &);

    MppCluster  *mClusters[VPU_CLIENT_BUTT];

public:
    static MppClusterServer *single() {
        static MppClusterServer inst;
        cluster_server = &inst;
        return &inst;
    }

    MppCluster  *get(MppClientType client_type);
    MPP_RET     put(MppClientType client_type);
};

MppClusterServer::MppClusterServer()
{
    memset(mClusters, 0, sizeof(mClusters));

    mpp_env_get_u32("mpp_cluster_debug", &mpp_cluster_debug, 0);
    mpp_env_get_u32("mpp_cluster_thd_cnt", &mpp_cluster_thd_cnt, 1);
}

MppClusterServer::~MppClusterServer()
{
    RK_S32 i;

    for (i = 0; i < VPU_CLIENT_BUTT; i++)
        put((MppClientType)i);
}

MppCluster *MppClusterServer::get(MppClientType client_type)
{
    RK_S32 i;
    MppCluster *p = NULL;

    if (client_type >= VPU_CLIENT_BUTT)
        goto done;

    {
        AutoMutex auto_lock(this);

        p = mClusters[client_type];
        if (p)
            goto done;

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

            mClusters[client_type] = p;
            cluster_dbg_flow("%s created\n", p->name);
        }
    }

done:
    if (p)
        cluster_dbg_flow("%s get\n", p->name);
    else
        cluster_dbg_flow("%d get cluster %d failed\n", getpid(), client_type);

    return p;
}

MPP_RET MppClusterServer::put(MppClientType client_type)
{
    RK_S32 i;

    if (client_type >= VPU_CLIENT_BUTT)
        return MPP_NOK;

    AutoMutex auto_lock(this);
    MppCluster *p = mClusters[client_type];

    if (!p)
        return MPP_NOK;

    for (i = 0; i < p->worker_count; i++)
        cluster_worker_deinit(&p->worker[i]);

    cluster_dbg_flow("put %s\n", p->name);

    mpp_free(p);

    return MPP_OK;
}

MPP_RET mpp_node_attach(MppNode node, MppClientType type)
{
    MppNodeImpl *impl = (MppNodeImpl *)node;
    MppCluster *p = MppClusterServer::single()->get(type);
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
